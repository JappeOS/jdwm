#include "output.hpp"
#include "server.hpp"
#include "embedder_callbacks.hpp"
#include "util/wlr/wlr_helpers.hpp"
#include "swap_chain.hpp"
#include "util/wlr/scoped_wlr_buffer.hpp"
#include "debug.hpp"
#include "cursor_debug.hpp"
#include "output/zenith_output_manager.hpp"
#include "output/rendering_policy.hpp"
#include "output/presentation_timing.hpp"
#include "output/frame_timing_trace.hpp"
#include "output/frame_selection.hpp"
#include "util/egl/gl_context_lock.hpp"
#include <unistd.h>
#include <atomic>
#include <cstdlib>
#include <cinttypes>
#include <vector>

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
#include <wlr/types/wlr_output_layout.h>
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
	// Flutter's clock must keep running while a page flip or a GPU fence is
	// pending. Repaint scheduling can defer to that flip and has separate
	// retry deadlines, so it cannot also serve as the animation clock.
	vsync_timer = wl_event_loop_add_timer(event_loop, [](void* data) {
		return vsync_callback(data);
	}, server);

	scene_output = wlr_scene_output_create(server->scene, wlr_output);
	scene_buffer = wlr_scene_buffer_create(&server->scene->tree, nullptr);
}

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

static bool output_source_box_in_extend_space(
	ZenithServer* server, ZenithOutput* output, struct wlr_box* source_box_out) {
	if (server == nullptr ||
	    output == nullptr ||
	    output->wlr_output == nullptr ||
	    server->output_layout == nullptr ||
	    source_box_out == nullptr) {
		return false;
	}

	struct wlr_box extents = {};
	struct wlr_box box = {};
	wlr_output_layout_get_box(server->output_layout, nullptr, &extents);
	wlr_output_layout_get_box(server->output_layout, output->wlr_output, &box);

	source_box_out->x = box.x - extents.x;
	source_box_out->y = box.y - extents.y;
	source_box_out->width = box.width;
	source_box_out->height = box.height;
	return box.width > 0 && box.height > 0;
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

static bool boxes_intersect(const wlr_box& a, const wlr_box& b) {
	return a.width > 0 && a.height > 0 && b.width > 0 && b.height > 0 &&
	       a.x < b.x + b.width &&
	       a.x + a.width > b.x &&
	       a.y < b.y + b.height &&
	       a.y + a.height > b.y;
}

static bool output_layout_box(ZenithServer* server, wlr_output* wlr_output, wlr_box* box_out) {
	if (server == nullptr || wlr_output == nullptr || box_out == nullptr) {
		return false;
	}
	if (server->output_layout != nullptr) {
		wlr_output_layout_get_box(server->output_layout, wlr_output, box_out);
	}
	if (box_out->width <= 0 || box_out->height <= 0) {
		box_out->x = 0;
		box_out->y = 0;
		box_out->width = wlr_output->width;
		box_out->height = wlr_output->height;
	}
	return box_out->width > 0 && box_out->height > 0;
}

static bool toplevel_intersects_output(ZenithServer* server, size_t view_id, const wlr_box& output_box) {
	if (server == nullptr) {
		return true;
	}
	auto geometry = server->toplevel_geometries.find(view_id);
	if (geometry == server->toplevel_geometries.end()) {
		// New clients may commit before Dart has sent placement geometry.
		// Keep the old behavior until we have enough data to filter safely.
		return true;
	}
	return boxes_intersect(geometry->second, output_box);
}

static void notify_legacy_clients_frame_done(ZenithServer* server, wlr_output* wlr_output) {
	if (server == nullptr) {
		return;
	}
	wlr_box output_box = {};
	if (!output_layout_box(server, wlr_output, &output_box)) {
		return;
	}

	timespec frame_done_now{};
	clock_gettime(CLOCK_MONOTONIC, &frame_done_now);
	for (auto& [id, view]: server->xdg_toplevels) {
		if (!toplevel_intersects_output(server, id, output_box)) {
			continue;
		}
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
		if (!toplevel_intersects_output(server, id, output_box)) {
			continue;
		}
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
		// Even an output that never became usable owns listeners and timers.
		output_destroy(&output->destroy, nullptr);
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
	zenith::render::FrameTimingScope frame_timer("output-frame", output->wlr_output->name);
	auto* server = ZenithServer::instance();

	log_cursor_mode_transition(output);

	auto source_swap_chain =
		server->output_manager->composition_source_swap_chain();
	if (source_swap_chain == nullptr || output->scene_output == nullptr || output->scene_buffer == nullptr) {
		wl_event_source_timer_update(output->schedule_frame_timer, 1);
		return;
	}

	ZenithOutput* vsync_output = server->output_manager->vsync_driver_output();
	const bool is_vsync_driver = (output == vsync_output);

	auto selection = zenith::render::select_output_frame(*source_swap_chain, output->schedule_frame_timer);
	auto source_slot = std::move(selection.slot);
	zenith::render::trace_frame_sample("latest-fence", output->wlr_output->name, 0, selection.latest_pending);
	if (source_slot == nullptr || source_slot->buffer == nullptr) {
		// Bootstrap: if we have no composed frame yet, still service one vsync
		// tick so Flutter can produce the first frame.
		if (is_vsync_driver) {
			vsync_callback(server, output->wlr_output);
		}
		wl_event_source_timer_update(output->schedule_frame_timer, 1);
		return;
	}
	const bool cursor_repaint_requested =
		output->cursor_frame_pending || software_cursor_active(output->wlr_output);
	const bool source_ready = source_slot->is_ready_nonblocking();
	zenith::render::trace_frame_sample("source-fence", output->wlr_output->name, 0, !source_ready);
	if (!source_ready) {
		// Cursor updates must not stall behind a not-yet-ready newest frame.
		// Reuse the already-presented slot to clear old cursor pixels.
		if (cursor_repaint_requested &&
		    output->presented_slot != nullptr &&
		    output->presented_slot->generation == source_swap_chain->damage_history.generation() &&
		    output->presented_slot->buffer != nullptr &&
		    output->presented_slot->is_ready_nonblocking()) {
			if (zenith_cursor_debug_enabled()) {
				wlr_log(
					WLR_INFO,
					"zenith:cursor frame output='%s' reusing presented slot serial=%" PRIu64,
					output->wlr_output->name,
					output->presented_slot->frame_serial.load(std::memory_order_acquire)
				);
			}
			source_slot = output->presented_slot;
		} else {
			// Back-pressure: don't request a new Flutter frame until the latest
			// composed frame is actually ready for scanout.
			wl_event_source_timer_update(output->schedule_frame_timer, 4);
			return;
		}
	}
	wlr_gles2_buffer* source_buffer = source_slot->buffer.get();
	const bool extend_mode = server->output_manager->mode() == multimonitor::MultiMonitorMode::Extend;
	const uint64_t source_frame_serial = source_slot->frame_serial.load(std::memory_order_acquire);
	const bool same_generation = output->last_source_generation == source_slot->generation;
	const bool has_new_source_frame = !same_generation || source_frame_serial > output->last_presented_source_serial;
	zenith::render::trace_frame_sample("source-repeat", output->wlr_output->name, 0, !has_new_source_frame);

	wlr_buffer* source_wlr_buffer = source_buffer->buffer;
	struct wlr_box source_box_i = {};
	struct wlr_fbox source_box_f = {};
	const bool have_source_box = extend_mode && output_source_box_in_extend_space(server, output, &source_box_i);
	if (have_source_box) {
		source_box_f = {
			.x = static_cast<double>(source_box_i.x),
			.y = static_cast<double>(source_box_i.y),
			.width = static_cast<double>(source_box_i.width),
			.height = static_cast<double>(source_box_i.height),
		};
	}

	bool source_mapping_changed = false;
	if (extend_mode && have_source_box) {
		source_mapping_changed =
			!output->has_last_source_box ||
			output->last_source_box.x != source_box_i.x ||
			output->last_source_box.y != source_box_i.y ||
			output->last_source_box.width != source_box_i.width ||
			output->last_source_box.height != source_box_i.height ||
			output->last_dest_width != source_box_i.width ||
			output->last_dest_height != source_box_i.height;
	}

	// An output may skip several Flutter frames. Repair from its own accepted
	// serial, not the selected slot's last frame delta. All snapshots are owned.
	auto damage = source_swap_chain->damage_since(output->last_source_generation,
		output->last_presented_source_serial, source_frame_serial);
	if (same_generation && source_frame_serial < output->last_presented_source_serial) {
		// Never regress output content. Cursor-only retries use the retained
		// displayed slot; intervening accepted frames had no output-local damage.
		if (!cursor_repaint_requested || !output->presented_slot) {
			if (is_vsync_driver) vsync_callback(server, output->wlr_output);
			return;
		}

		source_slot = output->presented_slot;
		source_wlr_buffer = source_slot->buffer->buffer;
		damage = {};
	}

	if (zenith::render::force_full_repaint())
		damage = source_swap_chain->damage_history.full(zenith::render::DamageReason::Forced);

	if (source_mapping_changed || output->last_scene_buffer == nullptr || output->cursor_cleanup_pending)
		damage = source_swap_chain->damage_history.full(zenith::render::DamageReason::Generation);

	const bool output_region_damaged = have_source_box
		? damage.intersects({double(source_box_i.x), double(source_box_i.y),
		                    double(source_box_i.x + source_box_i.width), double(source_box_i.y + source_box_i.height)})
		: !damage.empty();

	const bool force_present = !same_generation || output->last_scene_buffer == nullptr ||
		source_mapping_changed || output->cursor_frame_pending || output->cursor_cleanup_pending ||
		output->scene_commit_pending || software_cursor_active(output->wlr_output);

	if (!force_present && !output_region_damaged) {
		output->last_presented_source_serial = std::max(output->last_presented_source_serial, source_frame_serial);
		output->last_source_generation = source_slot->generation;
		// Callback-only client commits must progress even when no pixels changed.
		notify_legacy_clients_frame_done(server, output->wlr_output);
		if (is_vsync_driver)
			vsync_callback(server, output->wlr_output);
		return;
	}

	zenith::render::trace_damage("output", output->wlr_output->name, damage);
	pixman_region32_t region;
	pixman_region32_init(&region);
	if (!damage.empty()) {
		pixman_region32_union_rect(&region, &region, int(damage.rect.left), int(damage.rect.top),
			unsigned(damage.rect.right - damage.rect.left), unsigned(damage.rect.bottom - damage.rect.top));
	}

	wlr_scene_buffer_set_buffer_with_damage(output->scene_buffer, source_wlr_buffer, &region);
	output->scene_slot = source_slot;
	output->scene_commit_pending = true;
	pixman_region32_fini(&region);

	if (extend_mode) {
		if (have_source_box) {
			wlr_scene_buffer_set_source_box(output->scene_buffer, &source_box_f);
			wlr_scene_buffer_set_dest_size(output->scene_buffer, source_box_i.width, source_box_i.height);
		} else {
			wlr_scene_buffer_set_source_box(output->scene_buffer, nullptr);
			wlr_scene_buffer_set_dest_size(output->scene_buffer, 0, 0);
		}
	} else {
		wlr_scene_buffer_set_source_box(output->scene_buffer, nullptr);
		wlr_scene_buffer_set_dest_size(output->scene_buffer, 0, 0);
	}

	bool output_committed = false;
	{
		if (zenith::egl::should_defer_for_flutter_frame_rendering()) {
			wl_event_source_timer_update(output->schedule_frame_timer, 16);
			return;
		}
		zenith::egl::TryGlContextGuard gl_guard;
		if (!gl_guard.owns_lock()) {
			wl_event_source_timer_update(output->schedule_frame_timer, 16);
			return;
		}
		zenith::render::FrameTimingScope commit_timer("scene-commit", output->wlr_output->name);
		output_committed = wlr_scene_output_commit(output->scene_output, nullptr);
	}
	if (!output_committed) {
		// If committing fails for some reason, manually schedule a new frame, otherwise rendering stops completely.
		// After 1 ms because if we do it right away, it will saturate the event loop and no other
		// tasks will execute.
		std::cerr << "commit failed" << std::endl;
		wl_event_source_timer_update(output->schedule_frame_timer, 1);
		return;
	}
	output->last_scene_buffer = source_wlr_buffer;
	output->scene_commit_pending = false;
	output->cursor_frame_pending = false;
	output->cursor_cleanup_pending = false;
	output->last_presented_source_serial = same_generation
		? std::max(output->last_presented_source_serial, source_frame_serial) : source_frame_serial;
	output->last_source_generation = source_slot->generation;
	if (extend_mode && have_source_box) {
		output->last_source_box = source_box_i;
		output->has_last_source_box = true;
		output->last_dest_width = source_box_i.width;
		output->last_dest_height = source_box_i.height;
	} else if (!extend_mode) {
		output->has_last_source_box = false;
		output->last_dest_width = 0;
		output->last_dest_height = 0;
	}
	swap_presented_slot(output, source_slot);

	// Notify scene-managed surfaces that this output frame has been presented.
	timespec now{};
	clock_gettime(CLOCK_MONOTONIC, &now);
	output->last_frame_commit_ns =
		static_cast<uint64_t>(now.tv_sec) * 1'000'000'000ull + static_cast<uint64_t>(now.tv_nsec);
	wlr_scene_output_send_frame_done(output->scene_output, &now);
	notify_legacy_clients_frame_done(server, output->wlr_output);

	if (is_vsync_driver) {
		vsync_callback(server, output->wlr_output);
	}
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
		if (server->output_manager == nullptr ||
		    server->output_manager->mode() != multimonitor::MultiMonitorMode::Extend) {
			output->recreate_swapchain();
			if (output->swap_chain == nullptr) {
				wlr_log(WLR_ERROR, "zenith: output '%s' has no swapchain after state change", output->wlr_output->name);
			}
		} else {
			output->swap_chain.reset();
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

	auto* driver = server->output_manager->vsync_driver_output();
	if (driver == nullptr || !driver->wlr_output->enabled) {
		return 0;
	}
	uint64_t now = FlutterEngineGetCurrentTime();
	auto chain = server->output_manager->composition_source_swap_chain();
	const bool frame_ready = chain != nullptr && chain->latest_frame_ready();
	auto dispatch = embedder_state->poll_vsync(
		now, zenith::render::output_frame_interval_ns(timing_output), frame_ready);
	if (dispatch.delay_ns != 0 || dispatch.baton.has_value()) {
		zenith::render::trace_frame_sample("vsync-backpressure", driver->wlr_output->name, 0, dispatch.gpu_blocked);
	}
	if (dispatch.delay_ns != 0) {
		// A no-damage output callback can run immediately at event-loop idle.
		// Keep the baton pending until both refresh pacing and GPU admission
		// allow another frame. The dedicated timer also resumes without input.
		int delay_ms = static_cast<int>((dispatch.delay_ns + 999'999) / 1'000'000);
		wl_event_source_timer_update(driver->vsync_timer, delay_ms);
		zenith::render::trace_frame_sample("vsync-request", driver->wlr_output->name, 0, true);
	} else if (dispatch.baton.has_value()) {
		wl_event_source_timer_update(driver->vsync_timer, 0);
		zenith::render::trace_frame_sample("vsync-request", driver->wlr_output->name, 0, false);
		embedder_state->on_vsync(*dispatch.baton, now, dispatch.target_ns);
	}
	return 0;
}

std::shared_ptr<SwapChain<wlr_gles2_buffer>> create_output_swap_chain(
	wlr_output* wlr_output, int width_override, int height_override) {
	ZenithServer* server = ZenithServer::instance();

	zenith::egl::GlContextGuard gl_guard;
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
		// The shared wrapper now owns a consumer lock. Release the allocator's
		// producer reference so the last lease actually frees the allocation.
		wlr_buffer_drop(buf);
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

	// Last ownership may be released by the raster thread after a hotplug.
	// Buffer destruction belongs to the Wayland thread.
	return {new SwapChain<wlr_gles2_buffer>(buffers, width, height),
	        [server](SwapChain<wlr_gles2_buffer>* chain) {
		        server->callable_queue.enqueue([chain] { delete chain; });
	        }};
}

void output_destroy(wl_listener* listener, void* data) {
	ZenithOutput* output = wl_container_of(listener, output, destroy);
	auto* server = ZenithServer::instance();
	// The manager removes the owning vector entry during this callback.
	std::shared_ptr<ZenithOutput> keep_alive;
	for (const auto& item : server->outputs)
		if (item.get() == output) keep_alive = item;

	// wlroots emits output->events.destroy and then asserts all output listener
	// lists are empty in wlr_output_finish().
	wl_list_remove(&output->frame_listener.link);
	wl_list_remove(&output->request_state_listener.link);
	wl_list_remove(&output->destroy.link);

	if (output->schedule_frame_timer != nullptr) {
		wl_event_source_remove(output->schedule_frame_timer);
		output->schedule_frame_timer = nullptr;
	}
	if (output->vsync_timer != nullptr) {
		wl_event_source_remove(output->vsync_timer);
		output->vsync_timer = nullptr;
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

	output->scene_slot.reset();
	output->scene_commit_pending = false;
	output->last_scene_buffer = nullptr;
	output->cursor_frame_pending = false;
	output->cursor_cleanup_pending = false;
	output->cursor_visible_last_event = false;
	output->has_last_source_box = false;
	output->last_dest_width = 0;
	output->last_dest_height = 0;
	output->last_presented_source_serial = 0;
	output->last_frame_commit_ns = 0;
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

	if (server == nullptr ||
	    server->output_manager == nullptr ||
	    server->output_manager->mode() != multimonitor::MultiMonitorMode::Extend) {
		recreate_swapchain();
		if (swap_chain == nullptr) {
			wlr_log(WLR_ERROR, "zenith: failed to enable output '%s' due to swapchain init failure", wlr_output->name);
			return false;
		}
	} else {
		swap_chain.reset();
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
	auto new_swap_chain = create_output_swap_chain(wlr_output, width, height);
	if (new_swap_chain == nullptr) {
		wlr_log(WLR_ERROR, "zenith: failed to recreate swapchain for output '%s'", wlr_output->name);
		return;
	}

	swap_chain = std::move(new_swap_chain);
	release_presented_slot(this);
	swapchain_width = width;
	swapchain_height = height;
	last_scene_buffer = nullptr;
	cursor_frame_pending = false;
	cursor_cleanup_pending = false;
	cursor_visible_last_event = false;
	has_last_source_box = false;
	last_dest_width = 0;
	last_dest_height = 0;
	last_presented_source_serial = 0;
	last_frame_commit_ns = 0;
	if (scene_buffer != nullptr) {
		wlr_scene_buffer_set_buffer(scene_buffer, nullptr);
	}

	scene_slot.reset();
	scene_commit_pending = false;
}
