#include "zenith_output_manager.hpp"

#include "server.hpp"
#include "output.hpp"
#include "cursor_debug.hpp"
#include "presentation_timing.hpp"
#include <algorithm>
#include <cstdint>
#include <ctime>
#include <cstdlib>
#include <utility>

extern "C" {
#define static
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>
#define class wlroots_xwayland_class
#include <wlr/xwayland/xwayland.h>
#undef class
#undef static
}

namespace zenith {

static bool equals_ignore_case(const char* value, const char* expected) {
	if (value == nullptr || expected == nullptr) {
		return false;
	}
	while (*value != '\0' && *expected != '\0') {
		char a = *value;
		char b = *expected;
		if (a >= 'A' && a <= 'Z') {
			a = static_cast<char>(a - 'A' + 'a');
		}
		if (b >= 'A' && b <= 'Z') {
			b = static_cast<char>(b - 'A' + 'a');
		}
		if (a != b) {
			return false;
		}
		++value;
		++expected;
	}
	return *value == '\0' && *expected == '\0';
}

static VsyncDriverMode parse_vsync_driver_mode_from_env() {
	const char* value = std::getenv("ZENITH_VSYNC_OUTPUT");
	if (value == nullptr || value[0] == '\0') {
		return VsyncDriverMode::ActiveOutput;
	}
	if (equals_ignore_case(value, "render") || equals_ignore_case(value, "source")) {
		return VsyncDriverMode::RenderOutput;
	}
	if (equals_ignore_case(value, "active") || equals_ignore_case(value, "cursor")) {
		return VsyncDriverMode::ActiveOutput;
	}
	if (equals_ignore_case(value, "highest_refresh") || equals_ignore_case(value, "max_refresh") ||
	    equals_ignore_case(value, "fastest")) {
		return VsyncDriverMode::HighestRefresh;
	}
	return VsyncDriverMode::ActiveOutput;
}

static const char* vsync_mode_to_string(VsyncDriverMode mode) {
	switch (mode) {
		case VsyncDriverMode::RenderOutput:
			return "render";
		case VsyncDriverMode::ActiveOutput:
			return "active";
		case VsyncDriverMode::HighestRefresh:
			return "highest_refresh";
	}
	return "active";
}

static ZenithOutput* find_output_by_wlr_output(ZenithServer* server, struct wlr_output* wlr_output) {
	if (wlr_output == nullptr) {
		return nullptr;
	}
	for (const auto& output : server->outputs) {
		if (output != nullptr && output->wlr_output == wlr_output) {
			return output.get();
		}
	}
	return nullptr;
}

static bool output_has_visible_cursor(struct wlr_output* wlr_output) {
	if (wlr_output == nullptr) {
		return false;
	}
	struct wlr_output_cursor* cursor = nullptr;
	wl_list_for_each(cursor, &wlr_output->cursors, link) {
		if (cursor->enabled && cursor->visible) {
			return true;
		}
	}
	return false;
}

static void get_layout_extents_or_output(
	ZenithServer* server, ZenithOutput* fallback_output, int* width, int* height) {
	*width = 0;
	*height = 0;
	if (server->output_layout != nullptr) {
		struct wlr_box extents = {};
		wlr_output_layout_get_box(server->output_layout, nullptr, &extents);
		*width = extents.width;
		*height = extents.height;
	}
	if ((*width <= 0 || *height <= 0) && fallback_output != nullptr && fallback_output->wlr_output != nullptr) {
		*width = fallback_output->wlr_output->width;
		*height = fallback_output->wlr_output->height;
	}
}

static void update_xwayland_workareas_for_layout(ZenithServer* server) {
	if (server == nullptr || server->xwayland == nullptr || !server->xwayland_is_ready ||
	    server->output_layout == nullptr) {
		return;
	}

	struct wlr_box extents = {};
	wlr_output_layout_get_box(server->output_layout, nullptr, &extents);
	if (extents.width <= 0 || extents.height <= 0) {
		if (server->output == nullptr || server->output->wlr_output == nullptr) {
			return;
		}
		extents.x = 0;
		extents.y = 0;
		extents.width = server->output->wlr_output->width;
		extents.height = server->output->wlr_output->height;
	}

	// Workareas are expressed in virtual-desktop coordinates. Normalize to a
	// 0,0-origin viewport while keeping full desktop dimensions.
	struct wlr_box workarea = {
		.x = 0,
		.y = 0,
		.width = extents.width,
		.height = extents.height,
	};
	wlr_xwayland_set_workareas(server->xwayland, &workarea, 1);
	wlr_log(
		WLR_INFO,
		"zenith:xw-workarea layout_extents=(%d,%d,%d,%d) send_workarea=(%d,%d,%d,%d)",
		extents.x,
		extents.y,
		extents.width,
		extents.height,
		workarea.x,
		workarea.y,
		workarea.width,
		workarea.height
	);
}

static bool output_exists(ZenithServer* server, ZenithOutput* output) {
	if (server == nullptr || output == nullptr) {
		return false;
	}
	for (const auto& candidate : server->outputs) {
		if (candidate.get() == output && candidate->wlr_output->enabled) {
			return true;
		}
	}
	return false;
}

static uint64_t monotonic_now_ns() {
	timespec now{};
	clock_gettime(CLOCK_MONOTONIC, &now);
	return static_cast<uint64_t>(now.tv_sec) * 1'000'000'000ull + static_cast<uint64_t>(now.tv_nsec);
}

static ZenithOutput* pick_highest_refresh_output(ZenithServer* server) {
	if (server == nullptr) {
		return nullptr;
	}
	ZenithOutput* best_output = nullptr;
	int best_refresh = 0;
	for (const auto& output : server->outputs) {
		if (output == nullptr || output->wlr_output == nullptr || !output->wlr_output->enabled) {
			continue;
		}
		int refresh = output->wlr_output->refresh;
		if (refresh <= 0) {
			refresh = 60'000;
		}
		if (best_output == nullptr || refresh > best_refresh) {
			best_output = output.get();
			best_refresh = refresh;
		}
	}
	return best_output;
}

ZenithOutputManager::ZenithOutputManager(
	ZenithServer* server)
	: server_(server), vsync_mode_(parse_vsync_driver_mode_from_env()) {
	wlr_log(
		WLR_INFO,
		"zenith: vsync driver mode=%s (ZENITH_VSYNC_OUTPUT)",
		vsync_mode_to_string(vsync_mode_)
	);
}

void ZenithOutputManager::refresh_xwayland_workareas() const {
	update_xwayland_workareas_for_layout(server_);
}

void ZenithOutputManager::add_output_to_layout(ZenithServer* server, ZenithOutput* output) {
	wlr_box extents{};
	wlr_output_layout_get_box(server->output_layout, nullptr, &extents);
	const int right = extents.width ? extents.x + extents.width : 0;
	wlr_output_layout_output* layout_output =
		wlr_output_layout_add(server->output_layout, output->wlr_output, right, 0);
	if (layout_output != nullptr && server->scene_output_layout != nullptr && output->scene_output != nullptr) {
		wlr_scene_output_layout_add_output(server->scene_output_layout, layout_output, output->scene_output);
	}
}

void ZenithOutputManager::update_scene_node_positions(ZenithServer* server) {
	for (const auto& output : server->outputs) {
		if (output == nullptr || output->wlr_output == nullptr || output->scene_buffer == nullptr) {
			continue;
		}
		struct wlr_box box = {};
		wlr_output_layout_get_box(server->output_layout, output->wlr_output, &box);
		wlr_scene_node_set_position(&output->scene_buffer->node, box.x, box.y);
	}
}

void ZenithOutputManager::clear_composition_target() const {
	for (const auto& output : server_->outputs) {
		if (output->scene_buffer)
			wlr_scene_buffer_set_buffer(output->scene_buffer, nullptr);
		output->scene_slot.reset();
		if (output->presented_slot)
			output->presented_slot->release_presentation();
		output->presented_slot.reset();
	}

	composition_mapping_.clear();
	prepared_target_.reset();
	composition_swap_chain_.reset();
	publish_composition_target();
	composition_format_output_ = nullptr;
	composition_width_ = 0;
	composition_height_ = 0;
}

void ZenithOutputManager::ensure_composition_target() const {
	ZenithOutput* format_output = current_render_output();
	if (format_output == nullptr || format_output->wlr_output == nullptr) {
		clear_composition_target();
		return;
	}

	int width = 0;
	int height = 0;
	get_layout_extents_or_output(server_, format_output, &width, &height);
	width = int(std::ceil(width * double(server_->display_scale)));
	height = int(std::ceil(height * double(server_->display_scale)));
	if (width <= 0 || height <= 0) {
		return;
	}

	// A layout/scale/transform change starts a fresh coordinate generation,
	// even when the virtual framebuffer dimensions happen to stay unchanged.
	std::vector<OutputMapping> mapping;
	for (const auto& output : server_->outputs) {
		if (!output || !output->wlr_output)
			continue;
		wlr_box box{};
		wlr_output_layout_get_box(server_->output_layout, output->wlr_output, &box);
		mapping.push_back({box.x, box.y, box.width, box.height,
			output->wlr_output->scale, output->wlr_output->transform, output->wlr_output->enabled});
	}

	const bool format_changed = composition_format_output_ != format_output;
	const bool size_changed =
		composition_width_ != width || composition_height_ != height;
	if (composition_swap_chain_ == nullptr || format_changed || size_changed || mapping != composition_mapping_) {
		auto new_swap_chain = prepared_target_
			? std::move(prepared_target_)
			: create_output_swap_chain(format_output->wlr_output, width, height);

		if (new_swap_chain == nullptr) {
			wlr_log(
				WLR_ERROR,
				"zenith: failed to create extend composition target (format_output='%s', size=%dx%d)",
				format_output->wlr_output->name,
				width,
				height
			);
			clear_composition_target();
			return;
		}

		composition_swap_chain_ = std::move(new_swap_chain);
		publish_composition_target();
		composition_mapping_ = std::move(mapping);
		composition_format_output_ = format_output;
		composition_width_ = width;
		composition_height_ = height;

		for (const auto& output : server_->outputs) {
			if (output == nullptr) {
				continue;
			}

			if (output->presented_slot != nullptr) {
				output->presented_slot->release_presentation();
				output->presented_slot = nullptr;
			}

			if (output->scene_buffer)
				wlr_scene_buffer_set_buffer(output->scene_buffer, nullptr);

			output->scene_slot.reset();
			output->scene_commit_pending = false;
			output->last_scene_buffer = nullptr;
			output->has_last_source_box = false;
			output->last_dest_width = 0;
			output->last_dest_height = 0;
			output->last_presented_source_serial = 0;
			output->last_frame_commit_ns = 0;
		}

		wlr_log(
			WLR_INFO,
			"zenith: extend composition target format_output='%s' size=%dx%d",
			format_output->wlr_output->name,
			width,
			height
		);
	}
}

void ZenithOutputManager::send_virtual_desktop_metrics(ZenithServer* server) {
	if (server == nullptr || server->output_layout == nullptr) {
		return;
	}

	struct wlr_box extents = {};
	wlr_output_layout_get_box(server->output_layout, nullptr, &extents);

	const int width = server->output_manager->composition_source_width();
	const int height = server->output_manager->composition_source_height();
	if (width <= 0 || height <= 0)
		return;

	server->composition_source_height_cache.store(height, std::memory_order_release);
	if (server->embedder_state == nullptr) {
		return;
	}

	FlutterWindowMetricsEvent window_metrics = {};
	window_metrics.struct_size = sizeof(FlutterWindowMetricsEvent);
	window_metrics.width = width;
	window_metrics.height = height;
	window_metrics.pixel_ratio = server->display_scale;
	server->embedder_state->send_window_metrics(window_metrics);
}

void ZenithOutputManager::handle_output_added(const std::shared_ptr<ZenithOutput>& output) const {
	if (!output)
		return;

	if (output->wlr_output->enabled)
		add_output_to_layout(server_, output.get());

	configured_[output->wlr_output] = capture_output(output.get());
	if (!configure(configuration(), false)) {
		wlr_output_state state;
		wlr_output_state_init(&state);
		state.allow_reconfiguration = true;
		wlr_output_state_set_enabled(&state, false);
		if (!wlr_output_commit_state(output->wlr_output, &state))
			wlr_log(WLR_ERROR, "zenith: failed to disable unusable output '%s'", output->wlr_output->name);
		wlr_output_state_finish(&state);
		configured_[output->wlr_output] = capture_output(output.get());
		reconcile();
	}
}

void ZenithOutputManager::handle_output_removed(ZenithOutput* removed) const {
	configured_.erase(removed->wlr_output);
	wlr_output_layout_remove(server_->output_layout, removed->wlr_output);
	if (active_output_ == removed)
		active_output_ = nullptr;
	if (last_cursor_output_ == removed)
		last_cursor_output_ = nullptr;
	if (composition_format_output_ == removed)
		composition_format_output_ = nullptr;
	if (server_->output.get() == removed)
		server_->output.reset();

	server_->outputs.erase(std::remove_if(server_->outputs.begin(), server_->outputs.end(),
		[removed](const auto& item) { return item.get() == removed; }), server_->outputs.end());
	if (!server_->is_shutting_down())
		reconcile();
}

ZenithOutput* ZenithOutputManager::output_for_cursor(double x, double y) const {
	if (server_->output_layout == nullptr) {
		return server_->output.get();
	}
	struct wlr_output* wlr_output = wlr_output_layout_output_at(server_->output_layout, x, y);
	ZenithOutput* output = find_output_by_wlr_output(server_, wlr_output);
	return output != nullptr ? output : server_->output.get();
}

void ZenithOutputManager::schedule_cursor_frame(double x, double y) const {
	if (!output_exists(server_, last_cursor_output_)) {
		last_cursor_output_ = nullptr;
	}
	ZenithOutput* cursor_output = output_for_cursor(x, y);
	ZenithOutput* previous_output = last_cursor_output_;
	ZenithOutput* driver_output = vsync_driver_output();
	if (zenith_cursor_debug_enabled() && previous_output != cursor_output) {
		wlr_log(
			WLR_INFO,
			"zenith:cursor transition prev=%s next=%s driver=%s",
			(previous_output != nullptr && previous_output->wlr_output != nullptr)
				? previous_output->wlr_output->name
				: "<none>",
			(cursor_output != nullptr && cursor_output->wlr_output != nullptr)
				? cursor_output->wlr_output->name
				: "<none>",
			(driver_output != nullptr && driver_output->wlr_output != nullptr)
				? driver_output->wlr_output->name
				: "<none>"
		);
	}
	auto schedule_cursor_output = [this](ZenithOutput* output, bool cleanup_pending) {
		if (output == nullptr || output->wlr_output == nullptr || !output->wlr_output->enabled) {
			return;
		}
		output->cursor_frame_pending = true;
		output->cursor_cleanup_pending = output->cursor_cleanup_pending || cleanup_pending;
		schedule_output_frame(output, true);
	};

	for (const auto& output_ptr : server_->outputs) {
		ZenithOutput* output = output_ptr.get();
		if (output == nullptr || output->wlr_output == nullptr || !output->wlr_output->enabled) {
			continue;
		}

		const bool cursor_visible_now = output_has_visible_cursor(output->wlr_output);
		const bool cursor_became_invisible = output->cursor_visible_last_event && !cursor_visible_now;
		const bool is_cursor_output = output == cursor_output;
		const bool is_previous_output = output == previous_output;
		const bool is_driver_output = output == driver_output;

		const bool needs_schedule =
			is_cursor_output ||
			is_previous_output ||
			is_driver_output ||
			cursor_visible_now ||
			cursor_became_invisible;
		const bool cleanup_pending =
			cursor_became_invisible ||
			(is_previous_output && previous_output != cursor_output && !cursor_visible_now);

		if (needs_schedule) {
			schedule_cursor_output(output, cleanup_pending);
			if (zenith_cursor_debug_enabled() && cursor_became_invisible) {
				wlr_log(
					WLR_INFO,
					"zenith:cursor visibility dropped on output '%s' -> schedule cleanup",
					output->wlr_output->name
				);
			}
		}

		output->cursor_visible_last_event = cursor_visible_now;
	}
	last_cursor_output_ = cursor_output;
	return;
}

void ZenithOutputManager::schedule_compositor_frame() const {
	ZenithOutput* driver_output = vsync_driver_output();
	for (const auto& output : server_->outputs) {
		if (output != nullptr && output->wlr_output != nullptr) {
			schedule_output_frame(output.get(), output.get() == driver_output);
		}
	}
	return;
}

void ZenithOutputManager::set_display_enabled(bool enable) const {
	if (display_enabled_ == enable)
		return;

	auto proposal = configuration();
	const bool previous = display_enabled_;
	display_enabled_ = enable;
	if (!configure(proposal, false))
		display_enabled_ = previous;
}

void ZenithOutputManager::update_active_output_from_cursor(double x, double y) const {
	ZenithOutput* hovered = output_for_cursor(x, y);
	if (hovered != nullptr) {
		active_output_ = hovered;
	}
}

void ZenithOutputManager::schedule_output_frame(ZenithOutput* output, bool force_immediate) const {
	if (server_->is_shutting_down())
		return;

	if (output == nullptr || output->wlr_output == nullptr || !output->wlr_output->enabled) {
		return;
	}

	if (force_immediate) {
		wlr_output_schedule_frame(output->wlr_output);
		return;
	}

	const uint64_t interval_ns = zenith::render::output_frame_interval_ns(output->wlr_output);
	if (interval_ns == 0 || output->last_frame_commit_ns == 0) {
		wlr_output_schedule_frame(output->wlr_output);
		return;
	}

	const uint64_t now_ns = monotonic_now_ns();
	const uint64_t target_ns = output->last_frame_commit_ns + interval_ns;
	if (now_ns + 200'000ull >= target_ns) {
		wlr_output_schedule_frame(output->wlr_output);
		return;
	}

	const uint64_t delay_ns = target_ns - now_ns;
	int delay_ms = static_cast<int>(delay_ns / 1'000'000ull);
	if (delay_ms < 1) {
		delay_ms = 1;
	}
	if (output->schedule_frame_timer != nullptr) {
		wl_event_source_timer_update(output->schedule_frame_timer, delay_ms);
		return;
	}
	wlr_output_schedule_frame(output->wlr_output);
}

std::shared_ptr<SwapChain<wlr_gles2_buffer>> ZenithOutputManager::composition_source_swap_chain() const {
	return std::atomic_load(&published_target_);
}

void ZenithOutputManager::publish_composition_target() const {
	std::atomic_store(&published_target_, composition_swap_chain_);
}

int ZenithOutputManager::composition_source_width() const { return composition_width_; }

int ZenithOutputManager::composition_source_height() const { return composition_height_; }

ZenithOutput* ZenithOutputManager::current_render_output() const {
	if (server_->output && server_->output->wlr_output->enabled)
		return server_->output.get();

	for (const auto& output : server_->outputs)
		if (output && output->wlr_output->enabled)
			return output.get();

	return nullptr;
}

ZenithOutput* ZenithOutputManager::vsync_driver_output() const {
	if (!output_exists(server_, active_output_))
		active_output_ = nullptr;

	switch (vsync_mode_) {
		case VsyncDriverMode::RenderOutput:
			return current_render_output();
		case VsyncDriverMode::ActiveOutput:
			if (active_output_ != nullptr) {
				return active_output_;
			}
			return pick_highest_refresh_output(server_);
		case VsyncDriverMode::HighestRefresh:
			return pick_highest_refresh_output(server_);
	}
	return current_render_output();
}

} // namespace zenith
