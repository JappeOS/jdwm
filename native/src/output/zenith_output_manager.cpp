#include "zenith_output_manager.hpp"

#include "server.hpp"
#include "output.hpp"
#include <algorithm>

extern "C" {
#define static
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>
#undef static
}

namespace zenith {

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

static void ensure_extend_render_target(ZenithServer* server) {
	if (server->output == nullptr) {
		return;
	}
	int width = 0;
	int height = 0;
	get_layout_extents_or_output(server, server->output.get(), &width, &height);
	if (width <= 0 || height <= 0) {
		return;
	}
	if (server->output->swap_chain == nullptr ||
	    server->output->swapchain_width != width ||
	    server->output->swapchain_height != height) {
		server->output->recreate_swapchain(width, height);
		wlr_log(WLR_INFO, "zenith: extend render target resized to %dx%d", width, height);
	}
}

ZenithOutputManager::ZenithOutputManager(
	ZenithServer* server, multimonitor::MultiMonitorMode mode)
	: server_(server), mode_(mode) {
}

multimonitor::MultiMonitorMode ZenithOutputManager::mode() const {
	return mode_;
}

bool ZenithOutputManager::is_multi_output_enabled() const {
	return mode_ != multimonitor::MultiMonitorMode::Off;
}

void ZenithOutputManager::add_output_to_layout(ZenithServer* server, ZenithOutput* output) {
	wlr_output_layout_output* layout_output =
		wlr_output_layout_add_auto(server->output_layout, output->wlr_output);
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

void ZenithOutputManager::send_single_output_metrics(ZenithServer* server, ZenithOutput* output) {
	if (server->embedder_state == nullptr || output == nullptr || output->wlr_output == nullptr) {
		return;
	}
	FlutterWindowMetricsEvent window_metrics = {};
	window_metrics.struct_size = sizeof(FlutterWindowMetricsEvent);
	window_metrics.width = output->wlr_output->width;
	window_metrics.height = output->wlr_output->height;
	window_metrics.pixel_ratio = server->display_scale;
	server->embedder_state->send_window_metrics(window_metrics);
}

void ZenithOutputManager::send_virtual_desktop_metrics(ZenithServer* server) {
	if (server->embedder_state == nullptr || server->output_layout == nullptr) {
		return;
	}

	struct wlr_box extents = {};
	wlr_output_layout_get_box(server->output_layout, nullptr, &extents);

	int width = extents.width;
	int height = extents.height;
	if (width <= 0 || height <= 0) {
		if (server->output == nullptr || server->output->wlr_output == nullptr) {
			return;
		}
		width = server->output->wlr_output->width;
		height = server->output->wlr_output->height;
	}

	FlutterWindowMetricsEvent window_metrics = {};
	window_metrics.struct_size = sizeof(FlutterWindowMetricsEvent);
	window_metrics.width = width;
	window_metrics.height = height;
	window_metrics.pixel_ratio = server->display_scale;
	server->embedder_state->send_window_metrics(window_metrics);
}

void ZenithOutputManager::handle_output_added(const std::shared_ptr<ZenithOutput>& output) const {
	if (output == nullptr) {
		return;
	}

	if (mode_ == multimonitor::MultiMonitorMode::Extend) {
		add_output_to_layout(server_, output.get());
		update_scene_node_positions(server_);
		if (server_->output == nullptr) {
			server_->output = output;
		}
		ensure_extend_render_target(server_);
		send_virtual_desktop_metrics(server_);
		struct wlr_box extents = {};
		wlr_output_layout_get_box(server_->output_layout, nullptr, &extents);
		wlr_log(WLR_INFO,
			"zenith: multimonitor(extend) added output='%s' outputs=%zu layout=%dx%d@(%d,%d)",
			output->wlr_output->name, server_->outputs.size(),
			extents.width, extents.height, extents.x, extents.y);
		schedule_compositor_frame();
		if (server_->embedder_state != nullptr) {
			server_->embedder_state->publish_monitor_layout();
		}
		return;
	}

	// Single-output behavior (off/last): keep newest output as active.
	if (server_->outputs.size() >= 2) {
		const auto& previous = server_->outputs[server_->outputs.size() - 2];
		if (previous != nullptr) {
			previous->disable();
			wlr_output_layout_remove(server_->output_layout, previous->wlr_output);
		}
	}
	add_output_to_layout(server_, output.get());
	server_->output = output;
	send_single_output_metrics(server_, output.get());
	if (mode_ == multimonitor::MultiMonitorMode::Last) {
		wlr_log(WLR_INFO, "zenith: multimonitor(last) active output='%s' outputs=%zu",
		        output->wlr_output->name, server_->outputs.size());
	}
	schedule_compositor_frame();
	if (server_->embedder_state != nullptr) {
		server_->embedder_state->publish_monitor_layout();
	}
}

void ZenithOutputManager::handle_output_removed(ZenithOutput* removed_output) const {
	if (removed_output == nullptr) {
		return;
	}
	wlr_output_layout_remove(server_->output_layout, removed_output->wlr_output);

	auto it = std::remove_if(server_->outputs.begin(), server_->outputs.end(),
		[removed_output](const std::shared_ptr<ZenithOutput>& o) {
			return o.get() == removed_output;
		});
	server_->outputs.erase(it, server_->outputs.end());

	if (server_->outputs.empty()) {
		server_->output = nullptr;
		if (server_->embedder_state != nullptr) {
			server_->embedder_state->publish_monitor_layout();
		}
		return;
	}

	if (mode_ == multimonitor::MultiMonitorMode::Extend) {
		if (server_->output != nullptr && server_->output->wlr_output == removed_output->wlr_output) {
			server_->output = server_->outputs.front();
		}
		update_scene_node_positions(server_);
		ensure_extend_render_target(server_);
		send_virtual_desktop_metrics(server_);
		if (server_->embedder_state != nullptr) {
			server_->embedder_state->publish_monitor_layout();
		}
		return;
	}

	// Single-output behavior (off/last)
	const bool removed_active_output =
		server_->output != nullptr && server_->output->wlr_output == removed_output->wlr_output;
	if (!removed_active_output) {
		return;
	}
	server_->output = server_->outputs.back();
	if (server_->output == nullptr) {
		return;
	}
	if (!server_->output->enable()) {
		wlr_log(WLR_ERROR, "zenith: failed to enable fallback output '%s'", server_->output->wlr_output->name);
		return;
	}
	add_output_to_layout(server_, server_->output.get());
	send_single_output_metrics(server_, server_->output.get());
	if (server_->embedder_state != nullptr) {
		server_->embedder_state->publish_monitor_layout();
	}
}

void ZenithOutputManager::handle_output_state_changed(ZenithOutput* changed_output) const {
	if (changed_output == nullptr) {
		return;
	}
	if (mode_ == multimonitor::MultiMonitorMode::Extend) {
		update_scene_node_positions(server_);
		ensure_extend_render_target(server_);
		send_virtual_desktop_metrics(server_);
		if (server_->embedder_state != nullptr) {
			server_->embedder_state->publish_monitor_layout();
		}
		return;
	}
	if (server_->output != nullptr && server_->output->wlr_output == changed_output->wlr_output) {
		send_single_output_metrics(server_, changed_output);
		if (server_->embedder_state != nullptr) {
			server_->embedder_state->publish_monitor_layout();
		}
	}
}

ZenithOutput* ZenithOutputManager::output_for_cursor(double x, double y) const {
	if (mode_ != multimonitor::MultiMonitorMode::Extend) {
		return server_->output.get();
	}
	if (server_->output_layout == nullptr) {
		return server_->output.get();
	}
	struct wlr_output* wlr_output = wlr_output_layout_output_at(server_->output_layout, x, y);
	ZenithOutput* output = find_output_by_wlr_output(server_, wlr_output);
	return output != nullptr ? output : server_->output.get();
}

float ZenithOutputManager::pointer_scale_at(double x, double y) const {
	ZenithOutput* output = output_for_cursor(x, y);
	if (output == nullptr || output->wlr_output == nullptr) {
		return 1.0f;
	}
	return (float) output->wlr_output->scale;
}

void ZenithOutputManager::schedule_cursor_frame(double x, double y) const {
	if (mode_ == multimonitor::MultiMonitorMode::Extend) {
		ZenithOutput* output = output_for_cursor(x, y);
		if (output != nullptr && output->wlr_output != nullptr) {
			wlr_output_schedule_frame(output->wlr_output);
		}
		return;
	}
	if (server_->output != nullptr && server_->output->wlr_output != nullptr) {
		wlr_output_schedule_frame(server_->output->wlr_output);
	}
}

void ZenithOutputManager::schedule_compositor_frame() const {
	if (mode_ == multimonitor::MultiMonitorMode::Extend) {
		for (const auto& output : server_->outputs) {
			if (output != nullptr && output->wlr_output != nullptr) {
				wlr_output_schedule_frame(output->wlr_output);
			}
		}
		return;
	}
	if (server_->output != nullptr && server_->output->wlr_output != nullptr) {
		wlr_output_schedule_frame(server_->output->wlr_output);
	}
}

void ZenithOutputManager::set_display_enabled(bool enable) const {
	if (mode_ == multimonitor::MultiMonitorMode::Extend) {
		for (const auto& output : server_->outputs) {
			if (output == nullptr) {
				continue;
			}
			if (enable) {
				output->enable();
			} else {
				output->disable();
			}
		}
		return;
	}
	if (server_->output == nullptr) {
		return;
	}
	if (enable) {
		server_->output->enable();
	} else {
		server_->output->disable();
	}
}

void ZenithOutputManager::update_active_output_from_cursor(double x, double y) const {
	if (mode_ != multimonitor::MultiMonitorMode::Extend) {
		return;
	}
	(void) x;
	(void) y;
}

ZenithOutput* ZenithOutputManager::presentation_source_output(ZenithOutput* target_output) const {
	if (mode_ == multimonitor::MultiMonitorMode::Extend &&
	    server_->output != nullptr &&
	    server_->output->swap_chain != nullptr) {
		return server_->output.get();
	}
	return target_output;
}

ZenithOutput* ZenithOutputManager::current_render_output() const {
	if (server_->output != nullptr) {
		return server_->output.get();
	}
	if (!server_->outputs.empty()) {
		return server_->outputs.back().get();
	}
	return nullptr;
}

} // namespace zenith
