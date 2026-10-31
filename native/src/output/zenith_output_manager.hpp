#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include "output_configuration.hpp"
#include <unordered_map>
#include "swap_chain.hpp"
#include "third_party/embedder.h"

struct ZenithServer;
struct ZenithOutput;
struct wlr_output_state;

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
	explicit ZenithOutputManager(ZenithServer* server);
	OutputConfiguration configuration() const;
	bool configure(const OutputConfiguration& proposal, bool test_only) const;
	void reconcile() const;
	void layout_to_flutter(double& x, double& y) const;

	void handle_output_added(const std::shared_ptr<ZenithOutput>& output) const;
	void handle_output_removed(ZenithOutput* removed_output) const;
	bool request_state(ZenithOutput* output, const wlr_output_state& state) const;

	ZenithOutput* output_for_cursor(double x, double y) const;
	void schedule_cursor_frame(double x, double y) const;
	void schedule_compositor_frame() const;
	void set_display_enabled(bool enable) const;
	void refresh_xwayland_workareas() const;
	void update_active_output_from_cursor(double x, double y) const;

	std::shared_ptr<SwapChain<wlr_gles2_buffer>> composition_source_swap_chain() const;
	// Wayland thread publishes; raster thread only loads an owned snapshot.
	void publish_composition_target() const;
	int composition_source_width() const;
	int composition_source_height() const;
	ZenithOutput* current_render_output() const;
	ZenithOutput* vsync_driver_output() const;

private:
	struct OutputMapping {
		int x, y, width, height;
		float scale;
		int transform;
		bool enabled;
		bool operator==(const OutputMapping& other) const {
			return std::tie(x, y, width, height, scale, transform, enabled) ==
			       std::tie(other.x, other.y, other.width, other.height, other.scale, other.transform, other.enabled);
		}
	};

	mutable std::shared_ptr<SwapChain<wlr_gles2_buffer>> published_target_;
	mutable std::shared_ptr<SwapChain<wlr_gles2_buffer>> prepared_target_;
	ZenithServer* server_ = nullptr;
	mutable std::unordered_map<wlr_output*, OutputSettings> configured_;
	mutable bool applying_ = false;
	mutable bool display_enabled_ = true;
	OutputSettings capture_output(ZenithOutput* output) const;
	VsyncDriverMode vsync_mode_ = VsyncDriverMode::RenderOutput;
	mutable ZenithOutput* active_output_ = nullptr;
	mutable ZenithOutput* last_cursor_output_ = nullptr;
	mutable ZenithOutput* composition_format_output_ = nullptr;
	mutable std::shared_ptr<SwapChain<wlr_gles2_buffer>> composition_swap_chain_ = nullptr;
	mutable std::vector<OutputMapping> composition_mapping_;
	mutable int composition_width_ = 0;
	mutable int composition_height_ = 0;

	static void send_virtual_desktop_metrics(ZenithServer* server);
	static void add_output_to_layout(ZenithServer* server, ZenithOutput* output);
	static void update_scene_node_positions(ZenithServer* server);
	void schedule_output_frame(ZenithOutput* output, bool force_immediate) const;
	void ensure_composition_target() const;
	void clear_composition_target() const;
};

} // namespace zenith
