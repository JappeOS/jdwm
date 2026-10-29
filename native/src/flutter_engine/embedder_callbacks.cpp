#include "embedder_callbacks.hpp"
#include "embedder_state.hpp"
#include "server.hpp"
#include "output.hpp"
#include "rect.hpp"
#include "output/zenith_output_manager.hpp"
#include "output/frame_timing_trace.hpp"
#include "output/flutter_damage_policy.hpp"
#include "util/egl/egl_extensions.hpp"
#include "util/egl/gl_context_lock.hpp"

extern "C" {
#include <GLES3/gl3.h>
#include <EGL/eglext.h>
#include <wlr/render/egl.h>
#include <wlr/render/gles2.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/util/log.h>
// Private wlroots headers for wlr_egl_make_current/wlr_egl_unset_current
#include <render/egl.h>
}

#include <cassert>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <sys/eventfd.h>
#include <unistd.h>
#include <vector>

static std::shared_ptr<SwapChain<wlr_gles2_buffer>> current_composition_swap_chain(ZenithServer* server) {
	return server != nullptr && server->output_manager != nullptr
		       ? server->output_manager->composition_source_swap_chain()
		       : nullptr;
}

static int create_frame_ready_fence_fd() {
	ZenithServer* server = ZenithServer::instance();
	if (server == nullptr || server->renderer == nullptr) {
		return -1;
	}
	if (zenith::egl::gl_context_serialization_enabled()) {
		return -1;
	}
	if (eglCreateSyncKHR == nullptr ||
	    eglDestroySyncKHR == nullptr ||
	    eglDupNativeFenceFDANDROID == nullptr) {
		return -1;
	}

	wlr_egl* egl = wlr_gles2_renderer_get_egl(server->renderer);
	if (egl == nullptr) {
		return -1;
	}

	EGLDisplay display = wlr_egl_get_display(egl);
	const EGLint attribs[] = {EGL_NONE};
	EGLSyncKHR sync = eglCreateSyncKHR(display, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
	if (sync == EGL_NO_SYNC_KHR) {
		return -1;
	}

	glFlush();
	int fence_fd = eglDupNativeFenceFDANDROID(display, sync);
	eglDestroySyncKHR(display, sync);
	if (fence_fd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
		return -1;
	}
	return fence_fd;
}

bool flutter_make_current(void* userdata) {
	auto* state = static_cast<EmbedderState*>(userdata);
	if (state == nullptr || state->flutter_gl_context == nullptr) {
		return false;
	}
	return wlr_egl_make_current(state->flutter_gl_context, NULL);
}

bool flutter_clear_current(void* userdata) {
	auto* state = static_cast<EmbedderState*>(userdata);
	if (state == nullptr || state->flutter_gl_context == nullptr) {
		return false;
	}

	bool success = wlr_egl_unset_current(state->flutter_gl_context);
	zenith::egl::set_flutter_frame_rendering_active(false);
	return success;
}

static thread_local uint64_t flutter_frame_started_ns = 0;

uint32_t flutter_fbo_callback(void* userdata) {
	flutter_frame_started_ns = zenith::render::frame_trace_now();
	GLuint fbo = attach_framebuffer(static_cast<EmbedderState*>(userdata));
	zenith::egl::set_flutter_frame_rendering_active(
		zenith::egl::gl_context_serialization_enabled() && fbo != 0
	);
	return fbo;
}

GLuint attach_framebuffer(EmbedderState* state) {
	ZenithServer* server = ZenithServer::instance();
	auto swap_chain = current_composition_swap_chain(server);
	if (swap_chain == nullptr) {
		state->raster_target.reset();
		state->raster_fbo = 0;
		return 0;
	}

	// Only the raster thread may wait for a free, GPU-completed target. The
	// Wayland thread remains available for input, frame selection and flips.
	const uint64_t acquire_started = zenith::render::frame_trace_now();
	wlr_gles2_buffer* gles2_buffer = swap_chain->start_write(&state->stop_signal);
	zenith::render::trace_frame_sample("write-acquire", "flutter",
		zenith::render::frame_trace_now() - acquire_started);
	zenith::render::trace_frame_sample("write-slot", "flutter", 0, gles2_buffer == nullptr);
	if (gles2_buffer == nullptr) {
		state->raster_target.reset();
		state->raster_fbo = 0;
		return 0;
	}

	state->raster_target = std::move(swap_chain);
	state->raster_fbo = gles2_buffer->fbo;
	return state->raster_fbo;
}

bool flutter_present(void* userdata, const FlutterPresentInfo* present_info) {
	if (flutter_frame_started_ns != 0) {
		// Skia invokes the FBO callback immediately after present to prepare
		// the next target. This interval includes pacing/idle time, not just
		// rendering; don't label it as raster CPU duration.
		zenith::render::trace_frame_sample("flutter-frame-interval", "flutter",
			zenith::render::frame_trace_now() - flutter_frame_started_ns);
		flutter_frame_started_ns = 0;
	}
	zenith::render::FrameTimingScope present_timer("flutter-present", "flutter");
	auto* state = static_cast<EmbedderState*>(userdata);
	if (!state->raster_target || state->raster_fbo != present_info->fbo_id) {
		wlr_log(WLR_ERROR, "zenith: refusing publication from an unknown composition FBO");
		zenith::egl::set_flutter_frame_rendering_active(false);
		return false;
	}
	int ready_fence_fd = create_frame_ready_fence_fd();
	if (ready_fence_fd == -1) {
		static bool logged_sync_fallback = false;
		if (!logged_sync_fallback) {
			wlr_log(WLR_INFO, "zenith: using glFinish fallback in flutter_present");
			logged_sync_fallback = true;
		}
		glFinish();
	}

	bool success = commit_framebuffer(state, ready_fence_fd);
	zenith::egl::set_flutter_frame_rendering_active(false);
	return success;
}

bool commit_framebuffer(EmbedderState* state, int ready_fence_fd) {
	ZenithServer* server = ZenithServer::instance();
	auto swap_chain = std::move(state->raster_target);
	state->raster_fbo = 0;
	if (swap_chain == nullptr) {
		if (ready_fence_fd != -1) {
			close(ready_fence_fd);
		}
		return false;
	}

	auto frame_damage = zenith::render::flutter_composition_damage(swap_chain->damage_history);
	swap_chain->end_write({&frame_damage.rect, 1}, ready_fence_fd);

	// Ensure freshly rendered frames are presented even if no input event
	// (like pointer motion) occurs to trigger a frame.
	zenith::render::trace_damage("frame", "flutter", frame_damage);
	server->callable_queue.enqueue([server, swap_chain]() {
		if (server->output_manager != nullptr &&
		    server->output_manager->composition_source_swap_chain() == swap_chain) {
			// Select completed content independently on each output. Even an
			// unchanged output may owe client frame callbacks; skip GPU commits
			// there only after checking its accumulated damage.
			server->output_manager->schedule_compositor_frame();
		}
	});
	return true;
}

void flutter_vsync_callback(void* userdata, intptr_t baton) {
	auto* state = static_cast<EmbedderState*>(userdata);
	state->set_baton(baton);
	auto* server = ZenithServer::instance();
	if (server != nullptr && server->output_manager != nullptr) {
		server->callable_queue.enqueue([server] {
			if (server->output_manager != nullptr) {
				// Service the paced request even while the output is waiting
				// for a page flip. Frame publication schedules the repaint.
				vsync_callback(server);
			}
		});
	}
}

bool flutter_gl_external_texture_frame_callback(void* userdata, int64_t texture_id, size_t width, size_t height,
                                                FlutterOpenGLTexture* texture_out) {
	zenith::render::trace_frame_sample("texture-resolve", "flutter", 0);
	auto* state = static_cast<EmbedderState*>(userdata);
	// Snapshot acquisition does not wait for the Wayland event loop. Each
	// frame owns its buffer until Flutter releases this particular image.
	auto frame = state->external_textures.acquire(texture_id);
	if (!frame) {
		return false;
	}
	static const bool trace_textures = std::getenv("ZENITH_TRACE_TEXTURES") != nullptr;
	if (trace_textures) {
		wlr_log(WLR_INFO,
		        "zenith:texture id=%lld name=%u target=0x%x buffer=%zux%zu requested=%zux%zu shared=%d",
		        (long long) texture_id, frame->name, frame->target,
		        frame->width, frame->height, width, height, glIsTexture(frame->name));
	}

	texture_out->target = frame->target;
	texture_out->format = GL_RGBA8;
	texture_out->name = frame->name;
	// Flutter's requested size is the widget's size, not the buffer's pixel size
	// (especially with buffer scale or wp_viewporter).
	texture_out->width = frame->width;
	texture_out->height = frame->height;
	texture_out->user_data = new ExternalTextureRegistry::Frame(std::move(frame));

	texture_out->destruction_callback = [](void* user_data) {
		// The frame's deleter queues wlroots buffer destruction on Wayland.
		delete static_cast<ExternalTextureRegistry::Frame*>(user_data);
	};

	return true;
}

void flutter_platform_message_callback(const FlutterPlatformMessage* message, void* userdata) {
	auto* state = static_cast<EmbedderState*>(userdata);

	if (message->struct_size != sizeof(FlutterPlatformMessage)) {
		std::cerr << "ERROR: Invalid message size received. Expected: "
		          << sizeof(FlutterPlatformMessage) << " but received "
		          << message->struct_size;
		return;
	}

	state->message_dispatcher.HandleMessage(*message, [] {}, [] {});
}

bool flutter_make_resource_current(void* userdata) {
	auto* state = static_cast<EmbedderState*>(userdata);
	if (state == nullptr || state->flutter_resource_gl_context == nullptr) {
		return false;
	}
	return wlr_egl_make_current(state->flutter_resource_gl_context, NULL);
}

/*
 * The default rendering is done upside down for some reason.
 * This flips the rendering on the x-axis.
 */
FlutterTransformation flutter_surface_transformation(void* data) {
	auto* server = ZenithServer::instance();
	const double height = server
		? server->composition_source_height_cache.load(std::memory_order_acquire) : 0;

	return FlutterTransformation{
		  1.0, 0.0, 0.0, 0.0, -1.0, height, 0.0, 0.0, 1.0,
	};
}
