#pragma once

#include <memory>
#include "multimonitor/multi_monitor_mode.hpp"

struct ZenithServer;
struct ZenithOutput;

namespace zenith {

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
	void set_display_enabled(bool enable) const;
	void update_active_output_from_cursor(double x, double y) const;

	ZenithOutput* presentation_source_output(ZenithOutput* target_output) const;
	ZenithOutput* current_render_output() const;

private:
	ZenithServer* server_ = nullptr;
	multimonitor::MultiMonitorMode mode_ = multimonitor::MultiMonitorMode::Off;

	static void send_single_output_metrics(ZenithServer* server, ZenithOutput* output);
	static void send_virtual_desktop_metrics(ZenithServer* server);
	static void add_output_to_layout(ZenithServer* server, ZenithOutput* output);
	static void update_scene_node_positions(ZenithServer* server);
};

} // namespace zenith
