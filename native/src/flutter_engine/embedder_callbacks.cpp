#include "embedder_callbacks.hpp"
#include "embedder_state.hpp"
#include "server.hpp"
#include "rect.hpp"
#include "output/zenith_output_manager.hpp"
#include "util/egl/egl_extensions.hpp"
#include "util/egl/gl_context_lock.hpp"

extern "C" {
#include <GLES3/gl3.h>
#include <EGL/eglext.h>
#include <wlr/render/egl.h>
#include <wlr/render/gles2.h>
#include <wlr/util/log.h>
// Private wlroots headers for wlr_egl_make_current/wlr_egl_unset_current
#include <render/egl.h>
}

#include <cassert>
#include <iostream>
#include <sys/eventfd.h>
#include <unistd.h>
#include <vector>

static thread_local int flutter_gl_lock_depth = 0;

class FlutterGlLockReleaseForBlockingCall {
public:
	FlutterGlLockReleaseForBlockingCall() {
		while (flutter_gl_lock_depth > 0) {
			--flutter_gl_lock_depth;
			++released_depth_;
			zenith::egl::unlock_gl_context();
		}
	}

	~FlutterGlLockReleaseForBlockingCall() {
		for (int i = 0; i < released_depth_; i++) {
			if (zenith::egl::lock_gl_context()) {
				++flutter_gl_lock_depth;
			}
		}
	}

	FlutterGlLockReleaseForBlockingCall(const FlutterGlLockReleaseForBlockingCall&) = delete;
	FlutterGlLockReleaseForBlockingCall& operator=(const FlutterGlLockReleaseForBlockingCall&) = delete;

private:
	int released_depth_ = 0;
};

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

	bool locked_gl_context = zenith::egl::lock_gl_context();
	if (!wlr_egl_make_current(state->flutter_gl_context, NULL)) {
		if (locked_gl_context) {
			zenith::egl::unlock_gl_context();
		}
		return false;
	}

	if (locked_gl_context) {
		++flutter_gl_lock_depth;
	}
	return true;
}

bool flutter_clear_current(void* userdata) {
	auto* state = static_cast<EmbedderState*>(userdata);
	if (state == nullptr || state->flutter_gl_context == nullptr) {
		return false;
	}

	bool success = wlr_egl_unset_current(state->flutter_gl_context);
	if (flutter_gl_lock_depth > 0) {
		--flutter_gl_lock_depth;
		zenith::egl::unlock_gl_context();
	}
	return success;
}

uint32_t flutter_fbo_callback(void* userdata) {
	return attach_framebuffer();
}

GLuint attach_framebuffer() {
	ZenithServer* server = ZenithServer::instance();
	SwapChain<wlr_gles2_buffer>* swap_chain = current_composition_swap_chain(server);
	if (swap_chain == nullptr) {
		return 0;
	}
	wlr_gles2_buffer* gles2_buffer = swap_chain->start_write();
	if (gles2_buffer == nullptr) {
		return 0;
	}
	return gles2_buffer->fbo;
}

bool flutter_present(void* userdata, const FlutterPresentInfo* present_info) {
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
	ZenithServer* server = ZenithServer::instance();
	const int64_t& view_id = texture_id;
	channel<wlr_gles2_texture_attribs> texture_attribs{};

	server->callable_queue.enqueue([&]() {
		std::scoped_lock lock(state->buffer_chains_mutex);
		auto find_client_chain = [&]() -> std::shared_ptr<SurfaceBufferChain<wlr_buffer>> {
			auto it = state->buffer_chains_in_use.find(view_id);
			if (it != state->buffer_chains_in_use.end()) {
				return it->second;
			}
			it = server->surface_buffer_chains.find(view_id);
			if (it != server->surface_buffer_chains.end()) {
				state->buffer_chains_in_use[view_id] = it->second;
				return it->second;
			}
			return nullptr;
		};

		const auto& client_chain = find_client_chain();

		if (client_chain == nullptr) {
			texture_attribs.write({});
			return;
		}

		wlr_buffer* buffer = client_chain->start_read();
		assert(buffer != nullptr);

		wlr_texture* texture = wlr_client_buffer_get(buffer)->texture;
		assert(texture != nullptr);

		wlr_gles2_texture_attribs attribs{};
		wlr_gles2_texture_get_attribs(texture, &attribs);
		texture_attribs.write(attribs);
		return;
	});

	wlr_gles2_texture_attribs attribs = {};
	{
		FlutterGlLockReleaseForBlockingCall release_gl_lock;
		attribs = texture_attribs.read();
	}
	if (attribs.tex == 0) {
		return false;
	}

	texture_out->target = attribs.target;
	texture_out->format = GL_RGBA8;
	texture_out->name = attribs.tex;
	texture_out->user_data = (void*) view_id;

	texture_out->destruction_callback = [](void* user_data) {
		auto* server = ZenithServer::instance();
		auto view_id = reinterpret_cast<int64_t>(user_data);
		server->callable_queue.enqueue([=]() {
			std::scoped_lock lock(server->embedder_state->buffer_chains_mutex);

			auto& buffer_chains_in_use = server->embedder_state->buffer_chains_in_use;

			auto it = buffer_chains_in_use.find(view_id);
			if (it != buffer_chains_in_use.end()) {
				it->second->end_read();
			}
		});
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
	channel<double> height_chan = {};
	auto* server = ZenithServer::instance();
	server->callable_queue.enqueue([server, &height_chan] {
		int height =
			(server != nullptr && server->output_manager != nullptr)
				? server->output_manager->composition_source_height()
				: 0;
		height_chan.write(height);
	});
	double height = 0;
	{
		FlutterGlLockReleaseForBlockingCall release_gl_lock;
		height = height_chan.read();
	}

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
