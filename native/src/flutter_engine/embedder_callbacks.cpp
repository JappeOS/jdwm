#include "embedder_callbacks.hpp"
#include "embedder_state.hpp"
#include "server.hpp"
#include "rect.hpp"
#include "output/zenith_output_manager.hpp"
#include "output/frame_timing_trace.hpp"
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

static SwapChain<wlr_gles2_buffer>* current_composition_swap_chain(ZenithServer* server) {
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
	GLuint fbo = attach_framebuffer();
	zenith::egl::set_flutter_frame_rendering_active(
		zenith::egl::gl_context_serialization_enabled() && fbo != 0
	);
	return fbo;
}

GLuint attach_framebuffer() {
	ZenithServer* server = ZenithServer::instance();
	SwapChain<wlr_gles2_buffer>* swap_chain = current_composition_swap_chain(server);
	if (swap_chain == nullptr) {
		return 0;
	}
	wlr_gles2_buffer* gles2_buffer = swap_chain->start_write();
	zenith::render::trace_frame_sample("write-slot", "flutter", 0, gles2_buffer == nullptr);
	if (gles2_buffer == nullptr) {
		return 0;
	}
	return gles2_buffer->fbo;
}

bool flutter_present(void* userdata, const FlutterPresentInfo* present_info) {
	if (flutter_frame_started_ns != 0) {
		zenith::render::trace_frame_sample("flutter-render", "flutter",
			zenith::render::frame_trace_now() - flutter_frame_started_ns);
		flutter_frame_started_ns = 0;
	}
	zenith::render::FrameTimingScope present_timer("flutter-present", "flutter");
	array_view<FlutterRect> frame_damage(present_info->frame_damage.damage, present_info->frame_damage.num_rects);
	int ready_fence_fd = create_frame_ready_fence_fd();
	if (ready_fence_fd == -1) {
		static bool logged_sync_fallback = false;
		if (!logged_sync_fallback) {
			wlr_log(WLR_INFO, "zenith: using glFinish fallback in flutter_present");
			logged_sync_fallback = true;
		}
		glFinish();
	}

	bool success = commit_framebuffer(frame_damage, ready_fence_fd);
	zenith::egl::set_flutter_frame_rendering_active(false);
	return success;
}

bool commit_framebuffer(array_view<FlutterRect> damage, int ready_fence_fd) {
	ZenithServer* server = ZenithServer::instance();
	SwapChain<wlr_gles2_buffer>* swap_chain = current_composition_swap_chain(server);
	if (swap_chain == nullptr) {
		if (ready_fence_fd != -1) {
			close(ready_fence_fd);
		}
		return false;
	}
	swap_chain->end_write(damage, ready_fence_fd);
	// Ensure freshly rendered frames are presented even if no input event
	// (like pointer motion) occurs to trigger a frame.
	std::vector<FlutterRect> frame_damage(damage.begin(), damage.end());
	server->callable_queue.enqueue([server, frame_damage = std::move(frame_damage)]() {
		server->output_manager->schedule_compositor_frame(frame_damage);
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
				server->output_manager->schedule_compositor_frame();
			}
		});
	}
}

bool flutter_gl_external_texture_frame_callback(void* userdata, int64_t texture_id, size_t width, size_t height,
                                                FlutterOpenGLTexture* texture_out) {
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
	double height =
		server != nullptr
			? server->composition_source_height_cache.load(std::memory_order_acquire)
			: 0;

	return FlutterTransformation{
		  1.0, 0.0, 0.0, 0.0, -1.0, height, 0.0, 0.0, 1.0,
	};
}

void flutter_populate_existing_damage(void* user_data, intptr_t fbo_id, FlutterDamage* existing_damage) {
	ZenithServer* server = ZenithServer::instance();
	SwapChain<wlr_gles2_buffer>* swap_chain = current_composition_swap_chain(server);
	array_view<FlutterRect> damage_regions =
		(swap_chain != nullptr)
			? swap_chain->get_damage_regions()
			: array_view<FlutterRect>(nullptr, 0);

	// TODO: Who should free this object? Me or Flutter?
	// Also, I think Flutter's partial repaint mechanism is not completely implemented.
	// It only works with one rectangle. If I give it more than one, it just ignores them.
	// For this reason we just combine all damage regions into one rectangle.
	auto* union_region = new FlutterRect{};
	if (damage_regions.size() > 0) {
		*union_region = damage_regions[0];
		for (size_t i = 1; i < damage_regions.size(); i++) {
			*union_region = rect_union(*union_region, damage_regions[i]);
		}
	}

	existing_damage->struct_size = sizeof(FlutterDamage);
	existing_damage->num_rects = 1;
	existing_damage->damage = union_region;
}
