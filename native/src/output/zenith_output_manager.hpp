#pragma once

#include <memory>
#include <vector>
#include "multimonitor/multi_monitor_mode.hpp"
#include "swap_chain.hpp"
#include "third_party/embedder.h"

struct ZenithServer;
struct ZenithOutput;
extern "C" {
struct wlr_gles2_buffer;
}

namespace zenith {

enum class VsyncDriverMode {
	RenderOutput,
	ActiveOutput,
	HighestRefresh,
};

class ZenithOutputManager {
public:
	ZenithOutputManager(ZenithServer* server, multimonitor::MultiMonitorMode mode);

	multimonitor::MultiMonitorMode mode() const;
	bool is_multi_output_enabled() const;

	void handle_output_added(const std::shared_ptr<ZenithOutput>& output) const;
	void handle_output_removed(ZenithOutput* removed_output) const;
	void handle_output_state_changed(ZenithOutput* changed_output) const;

	ZenithOutput* output_for_cursor(double x, double y) const;
	float pointer_scale_at(double x, double y) const;
	void schedule_cursor_frame(double x, double y) const;
	void schedule_compositor_frame() const;
	void schedule_compositor_frame(const std::vector<FlutterRect>& frame_damage) const;
	void set_display_enabled(bool enable) const;
	void refresh_xwayland_workareas() const;
	void update_active_output_from_cursor(double x, double y) const;

	SwapChain<wlr_gles2_buffer>* composition_source_swap_chain() const;
	int composition_source_width() const;
	int composition_source_height() const;
	ZenithOutput* current_render_output() const;
	ZenithOutput* vsync_driver_output() const;

private:
	ZenithServer* server_ = nullptr;
	multimonitor::MultiMonitorMode mode_ = multimonitor::MultiMonitorMode::Off;
	VsyncDriverMode vsync_mode_ = VsyncDriverMode::RenderOutput;
	mutable ZenithOutput* active_output_ = nullptr;
	mutable ZenithOutput* last_cursor_output_ = nullptr;
	mutable ZenithOutput* extend_composition_format_output_ = nullptr;
	mutable std::unique_ptr<SwapChain<wlr_gles2_buffer>> extend_composition_swap_chain_ = nullptr;
	mutable int extend_composition_width_ = 0;
	mutable int extend_composition_height_ = 0;

	static void send_single_output_metrics(ZenithServer* server, ZenithOutput* output);
	static void send_virtual_desktop_metrics(ZenithServer* server);
	static void add_output_to_layout(ZenithServer* server, ZenithOutput* output);
	static void update_scene_node_positions(ZenithServer* server);
	void schedule_output_frame(ZenithOutput* output, bool force_immediate) const;
	void ensure_composition_target() const;
};

} // namespace zenith
