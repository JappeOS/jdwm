#include "output.hpp"
#include "server.hpp"
#include "embedder_callbacks.hpp"
#include "util/wlr/wlr_helpers.hpp"
#include "swap_chain.hpp"
#include "util/wlr/scoped_wlr_buffer.hpp"
#include "debug.hpp"
#include "output/zenith_output_manager.hpp"
#include "output/rendering_policy.hpp"
#include "output/presentation_timing.hpp"
#include <unistd.h>
#include <cstdlib>

extern "C" {
#include <libdrm/drm_fourcc.h>
#include <GLES2/gl2ext.h>
#define static
#include <wlr/render/gles2.h>
#include <wlr/util/log.h>
#include <wlr/backend/drm.h>
#include <wlr/render/allocator.h>
#include <wlr/render/drm_format_set.h>
#include <wlr/render/interface.h>
#include <wlr/types/wlr_scene.h>
#define class wlroots_xwayland_class
#include <wlr/xwayland/xwayland.h>
#undef class
#undef static
}

ZenithOutput::ZenithOutput(struct wlr_output* wlr_output)
	  : wlr_output(wlr_output) {

	auto* server = ZenithServer::instance();

	frame_listener.notify = output_frame;
	wl_signal_add(&wlr_output->events.frame, &frame_listener);
	request_state_listener.notify = output_request_state;
	wl_signal_add(&wlr_output->events.request_state, &request_state_listener);
	destroy.notify = output_destroy;
	wl_signal_add(&wlr_output->events.destroy, &destroy);

	wl_event_loop* event_loop = wl_display_get_event_loop(server->display);
	schedule_frame_timer = wl_event_loop_add_timer(event_loop, [](void* data) {
		auto* output = static_cast<struct wlr_output*>(data);
		wlr_output_schedule_frame(output);
		return 0;
	}, wlr_output);

	scene_output = wlr_scene_output_create(server->scene, wlr_output);
	scene_buffer = wlr_scene_buffer_create(&server->scene->tree, nullptr);
}

static std::unique_ptr<SwapChain<wlr_gles2_buffer>> create_swap_chain(
	wlr_output* wlr_output, int width_override = 0, int height_override = 0);

static bool software_cursor_active(wlr_output* wlr_output) {
	struct wlr_output_cursor* cur;
	wl_list_for_each(cur, &wlr_output->cursors, link) {
		if (cur->enabled && cur->visible && wlr_output->hardware_cursor != cur) {
			return true;
		}
	}
	return false;
}

static int visible_cursor_count(wlr_output* wlr_output) {
	int count = 0;
	struct wlr_output_cursor* cur;
	wl_list_for_each(cur, &wlr_output->cursors, link) {
		if (cur->enabled && cur->visible) {
			count++;
		}
	}
	return count;
}

static void log_cursor_mode_transition(ZenithOutput* output) {
	wlr_output* wlr_output = output->wlr_output;
	bool software_active = software_cursor_active(wlr_output);
	if (output->cursor_mode_logged && output->last_software_cursor_active == software_active) {
		return;
	}

	output->cursor_mode_logged = true;
	output->last_software_cursor_active = software_active;

	if (software_active) {
		const char* reason = "a visible cursor is not using a hardware cursor plane";
		if (output->software_cursor_locked) {
			reason = "software cursor was forced (ZENITH_FORCE_SOFTWARE_CURSOR=1)";
		} else if (wlr_output->hardware_cursor == nullptr) {
			reason = "wlroots did not assign a hardware cursor";
		} else if (!wlr_output->hardware_cursor->enabled || !wlr_output->hardware_cursor->visible) {
			reason = "hardware cursor exists but is not enabled/visible";
		}

		wlr_log(WLR_INFO,
		        "zenith: software cursor active on output '%s' (reason: %s, visible cursors=%d)",
		        wlr_output->name, reason, visible_cursor_count(wlr_output));
		return;
	}

	if (visible_cursor_count(wlr_output) > 0) {
		wlr_log(WLR_INFO,
		        "zenith: hardware cursor active on output '%s' (visible cursors=%d)",
		        wlr_output->name, visible_cursor_count(wlr_output));
	}
}

static void set_scene_buffer_with_damage(
	ZenithOutput* output, wlr_buffer* buffer, SwapChain<wlr_gles2_buffer>* damage_source) {
	if (output->scene_buffer == nullptr) {
		return;
	}
	(void)damage_source;
	// Force a full scene-buffer update. The current partial-damage propagation
	// can desynchronize with Flutter's render target history and leave stale
	// pixels, which is especially visible on blurred widgets.
	wlr_scene_buffer_set_buffer(output->scene_buffer, buffer);
}

static void release_presented_slot(ZenithOutput* output) {
	if (output == nullptr || output->presented_slot == nullptr) {
		return;
	}
	output->presented_slot->release_presentation();
	output->presented_slot = nullptr;
}

static void swap_presented_slot(
	ZenithOutput* output, const std::shared_ptr<Slot<wlr_gles2_buffer>>& new_slot) {
	if (output == nullptr || output->presented_slot == new_slot) {
		return;
	}
	if (new_slot != nullptr) {
		new_slot->acquire_presentation();
	}
	release_presented_slot(output);
	output->presented_slot = new_slot;
}

static void free_drm_format(struct wlr_drm_format* format) {
	if (format == nullptr) {
		return;
	}
	free(format->modifiers);
	free(format);
}

void output_create_handle(wl_listener* listener, void* data) {
	ZenithServer* server = wl_container_of(listener, server, new_output);
	auto* wlr_output = static_cast<struct wlr_output*>(data);

	/* Configures the output created by the backend to use our allocator and our renderer */
	if (!wlr_output_init_render(wlr_output, server->allocator, server->renderer)) {
		return;
	}

	auto output = std::make_shared<ZenithOutput>(wlr_output);
	if (!output->enable()) {
		return;
	}
	wlr_log(WLR_INFO, "zenith: new output '%s' enabled (%dx%d)",
	        wlr_output->name, wlr_output->width, wlr_output->height);

	server->outputs.push_back(output);
	server->output_manager->handle_output_added(output);
}

void output_frame(wl_listener* listener, void* data) {
	(void)data;
	ZenithOutput* output = wl_container_of(listener, output, frame_listener);
	auto* server = ZenithServer::instance();

	log_cursor_mode_transition(output);

	ZenithOutput* source_output = server->output_manager->presentation_source_output(output);
	if (source_output == nullptr ||
	    source_output->swap_chain == nullptr ||
	    output->scene_output == nullptr ||
	    output->scene_buffer == nullptr) {
		wl_event_source_timer_update(output->schedule_frame_timer, 1);
		return;
	}

	ZenithOutput* vsync_output = server->output_manager->vsync_driver_output();
	if (output == vsync_output) {
		timespec frame_done_now{};
		clock_gettime(CLOCK_MONOTONIC, &frame_done_now);
		for (auto& [id, view]: server->xdg_toplevels) {
			(void)id;
			wlr_xdg_surface* xdg_surface = view->xdg_toplevel->base;
			if (!xdg_surface->surface->mapped || !view->visible()) {
				continue;
			}

			wlr_xdg_surface_for_each_surface(
				xdg_surface,
				[](struct wlr_surface* surface, int sx, int sy, void* data) {
					(void)sx;
					(void)sy;
					auto* now = static_cast<timespec*>(data);
					wlr_surface_send_frame_done(surface, now);
				},
				&frame_done_now
			);
		}

		for (auto& [id, view]: server->xwayland_toplevels) {
			(void)id;
			wlr_surface* surface = view->xwayland_surface->surface;
			if (surface == nullptr || !surface->mapped || !view->visible()) {
				continue;
			}
			wlr_surface_for_each_surface(
				surface,
				[](struct wlr_surface* child, int sx, int sy, void* data) {
					(void)sx;
					(void)sy;
					auto* now = static_cast<timespec*>(data);
					wlr_surface_send_frame_done(child, now);
				},
				&frame_done_now
			);
		}

		vsync_callback(server, output->wlr_output);
	}

	std::shared_ptr<Slot<wlr_gles2_buffer>> source_slot = source_output->swap_chain->start_read_slot();
	if (source_slot == nullptr || source_slot->buffer == nullptr) {
		wl_event_source_timer_update(output->schedule_frame_timer, 1);
		return;
	}
	wlr_gles2_buffer* source_buffer = source_slot->buffer.get();

	wlr_buffer* source_wlr_buffer = source_buffer->buffer;
	if (server->output_manager->mode() == multimonitor::MultiMonitorMode::Extend) {
		// In extended desktop mode, each output samples a cropped source box from
		// one shared framebuffer. Reusing partial damage here can miss updates on
		// the second monitor due to coordinate-space mismatch, causing trails.
		wlr_scene_buffer_set_buffer(output->scene_buffer, source_wlr_buffer);
	} else {
		set_scene_buffer_with_damage(output, source_wlr_buffer, source_output->swap_chain.get());
	}

	if (server->output_manager->mode() == multimonitor::MultiMonitorMode::Extend) {
		struct wlr_box extents = {};
		struct wlr_box box = {};
		wlr_output_layout_get_box(server->output_layout, nullptr, &extents);
		wlr_output_layout_get_box(server->output_layout, output->wlr_output, &box);
		struct wlr_fbox source_box = {
			.x = (double) (box.x - extents.x),
			.y = (double) (box.y - extents.y),
			.width = (double) box.width,
			.height = (double) box.height,
		};
		wlr_scene_buffer_set_source_box(output->scene_buffer, &source_box);
		wlr_scene_buffer_set_dest_size(output->scene_buffer, box.width, box.height);
	} else {
		wlr_scene_buffer_set_source_box(output->scene_buffer, nullptr);
		wlr_scene_buffer_set_dest_size(output->scene_buffer, 0, 0);
	}

	if (!wlr_scene_output_commit(output->scene_output, nullptr)) {
		// If committing fails for some reason, manually schedule a new frame, otherwise rendering stops completely.
		// After 1 ms because if we do it right away, it will saturate the event loop and no other
		// tasks will execute.
		std::cerr << "commit failed" << std::endl;
		wl_event_source_timer_update(output->schedule_frame_timer, 1);
		return;
	}
	output->last_scene_buffer = source_wlr_buffer;
	swap_presented_slot(output, source_slot);

	// Notify scene-managed surfaces that this output frame has been presented.
	timespec now{};
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(output->scene_output, &now);
}

void output_request_state(wl_listener* listener, void* data) {
	ZenithOutput* output = wl_container_of(listener, output, request_state_listener);
	auto* event = static_cast<wlr_output_event_request_state*>(data);
	auto* server = ZenithServer::instance();

	if (!wlr_output_commit_state(output->wlr_output, event->state)) {
		wlr_log(WLR_ERROR, "zenith: failed to apply requested state for output '%s'", output->wlr_output->name);
		return;
	}

	if (output->wlr_output->enabled) {
		output->recreate_swapchain();
		if (output->swap_chain == nullptr) {
			wlr_log(WLR_ERROR, "zenith: output '%s' has no swapchain after state change", output->wlr_output->name);
		}
	} else {
		release_presented_slot(output);
	}

	server->output_manager->handle_output_state_changed(output);
}

int vsync_callback(void* data) {
	auto* server = static_cast<ZenithServer*>(data);
	if (server == nullptr || server->output_manager == nullptr) {
		return 0;
	}
	ZenithOutput* timing = server->output_manager->vsync_driver_output();
	return vsync_callback(data, timing != nullptr ? timing->wlr_output : nullptr);
}

int vsync_callback(void* data, struct wlr_output* timing_output) {
	auto* server = static_cast<ZenithServer*>(data);
	if (server == nullptr) {
		return 0;
	}
	auto& embedder_state = server->embedder_state;
	if (embedder_state == nullptr) {
		return 0;
	}

	/*
	 * Notify the compositor to prepare a new frame for the next time.
	 */
	std::optional<intptr_t> baton = embedder_state->get_baton();
	if (baton.has_value()) {
		uint64_t now = FlutterEngineGetCurrentTime();
		uint64_t next_frame = zenith::render::next_presentation_time_ns(now, timing_output);
		embedder_state->on_vsync(*baton, now, next_frame);
	}
	return 0;
}

std::unique_ptr<SwapChain<wlr_gles2_buffer>> create_swap_chain(
	wlr_output* wlr_output, int width_override, int height_override) {
	ZenithServer* server = ZenithServer::instance();

	wlr_egl_make_current(wlr_gles2_renderer_get_egl(server->renderer), NULL);

	const size_t buffer_count = zenith::render::desired_swapchain_buffer_count(server);
	std::vector<std::shared_ptr<wlr_gles2_buffer>> buffers;
	buffers.reserve(buffer_count);
	const int width = width_override > 0 ? width_override : wlr_output->width;
	const int height = height_override > 0 ? height_override : wlr_output->height;

	wlr_drm_format* drm_format = get_output_format(wlr_output);
	if (drm_format == nullptr) {
		wlr_log(WLR_ERROR, "zenith: failed to pick output format for '%s'", wlr_output->name);
		return nullptr;
	}
	for (size_t i = 0; i < buffer_count; i++) {
		wlr_buffer* buf = wlr_allocator_create_buffer(server->allocator, width, height,
		                                              drm_format);
		if (buf == nullptr) {
			wlr_log(WLR_ERROR, "zenith: failed to allocate swapchain buffer %zu for output '%s'", i, wlr_output->name);
			break;
		}
		assert(wlr_renderer_is_gles2(server->renderer));
		auto* gles2_renderer = (struct wlr_gles2_renderer*) server->renderer;
		wlr_gles2_buffer* gles2_buffer = create_buffer(gles2_renderer, buf);
		if (gles2_buffer == nullptr) {
			wlr_log(WLR_ERROR, "zenith: failed to create GLES2 wrapper for swapchain buffer %zu on '%s'", i, wlr_output->name);
			wlr_buffer_drop(buf);
			break;
		}
		buffers.emplace_back(scoped_wlr_gles2_buffer(gles2_buffer));
	}
	free_drm_format(drm_format);
	if (buffers.size() < 4) {
		wlr_log(WLR_ERROR,
		        "zenith: swapchain creation for output '%s' produced only %zu buffers (need >=4)",
		        wlr_output->name, buffers.size());
		return nullptr;
	}

	wlr_log(WLR_INFO, "zenith: swapchain output='%s' size=%dx%d buffers=%zu",
	        wlr_output->name, width, height, buffers.size());
	return std::make_unique<SwapChain<wlr_gles2_buffer>>(buffers);
}

void output_destroy(wl_listener* listener, void* data) {
	ZenithOutput* output = wl_container_of(listener, output, destroy);
	auto* server = ZenithServer::instance();

	// wlroots emits output->events.destroy and then asserts all output listener
	// lists are empty in wlr_output_finish().
	wl_list_remove(&output->frame_listener.link);
	wl_list_remove(&output->request_state_listener.link);
	wl_list_remove(&output->destroy.link);

	if (output->schedule_frame_timer != nullptr) {
		wl_event_source_remove(output->schedule_frame_timer);
		output->schedule_frame_timer = nullptr;
	}

	if (output->software_cursor_locked) {
		wlr_output_lock_software_cursors(output->wlr_output, false);
		output->software_cursor_locked = false;
	}
	if (output->attach_render_locked) {
		wlr_output_lock_attach_render(output->wlr_output, false);
		output->attach_render_locked = false;
	}

	if (output->scene_buffer != nullptr) {
		wlr_scene_node_destroy(&output->scene_buffer->node);
		output->scene_buffer = nullptr;
	}
	if (output->scene_output != nullptr) {
		wlr_scene_output_destroy(output->scene_output);
		output->scene_output = nullptr;
	}
	output->last_scene_buffer = nullptr;
	release_presented_slot(output);

	server->output_manager->handle_output_removed(output);
}

bool ZenithOutput::enable() {
	struct wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_enabled(&state, true);
	// Set the preferred resolution and refresh rate of the monitor which will probably be the highest one.
	wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output);
	wlr_output_state_set_mode(&state, mode);
	wlr_output_state_set_scale(&state, ZenithServer::instance()->display_scale);

	if (!wlr_output_commit_state(wlr_output, &state)) {
		wlr_output_state_finish(&state);
		std::cout << "commit failed" << std::endl;
		return false;
	}
	wlr_output_state_finish(&state);

	auto* server = ZenithServer::instance();
	if (server != nullptr &&
	    server->output_manager != nullptr &&
	    server->output_manager->mode() == multimonitor::MultiMonitorMode::Extend &&
	    !zenith::render::allow_direct_scanout() &&
	    !attach_render_locked) {
		wlr_output_lock_attach_render(wlr_output, true);
		attach_render_locked = true;
		wlr_log(
			WLR_INFO,
			"zenith: disabled direct scan-out on output '%s' in extend mode",
			wlr_output->name
		);
	}

	// Optional: force software cursor composition on DRM/tty for debugging
	// problematic hardware cursor planes.
	if (wlr_output_is_drm(wlr_output) && zenith::render::force_software_cursor() && !software_cursor_locked) {
		wlr_output_lock_software_cursors(wlr_output, true);
		software_cursor_locked = true;
		wlr_log(WLR_INFO, "zenith: forced software cursor lock on output '%s'", wlr_output->name);
	}

	recreate_swapchain();
	if (swap_chain == nullptr) {
		wlr_log(WLR_ERROR, "zenith: failed to enable output '%s' due to swapchain init failure", wlr_output->name);
		return false;
	}
	return true;
}

bool ZenithOutput::disable() {
	struct wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_enabled(&state, false);
	if (!wlr_output_commit_state(wlr_output, &state)) {
		wlr_output_state_finish(&state);
		std::cout << "commit failed" << std::endl;
		return false;
	}
	wlr_output_state_finish(&state);
	if (attach_render_locked) {
		wlr_output_lock_attach_render(wlr_output, false);
		attach_render_locked = false;
	}
	if (software_cursor_locked) {
		wlr_output_lock_software_cursors(wlr_output, false);
		software_cursor_locked = false;
	}
	release_presented_slot(this);
	return true;
}

void ZenithOutput::recreate_swapchain() {
	recreate_swapchain(wlr_output->width, wlr_output->height);
}

void ZenithOutput::recreate_swapchain(int width, int height) {
	auto new_swap_chain = create_swap_chain(wlr_output, width, height);
	if (new_swap_chain == nullptr) {
		wlr_log(WLR_ERROR, "zenith: failed to recreate swapchain for output '%s'", wlr_output->name);
		return;
	}
	swap_chain = std::move(new_swap_chain);
	release_presented_slot(this);
	swapchain_width = width;
	swapchain_height = height;
	last_scene_buffer = nullptr;
	if (scene_buffer != nullptr) {
		wlr_scene_buffer_set_buffer(scene_buffer, nullptr);
	}
}
