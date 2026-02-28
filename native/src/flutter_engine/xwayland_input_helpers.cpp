#include "xwayland_input_helpers.hpp"

#include <algorithm>
#include <cmath>

#include "server.hpp"
#include "output/zenith_output_manager.hpp"
#include "xwayland_debug.hpp"

extern "C" {
#define static
#include "wlr/types/wlr_output_layout.h"
#include "wlr/types/wlr_seat.h"
#include "wlr/util/log.h"
#define class wlroots_xwayland_class
#include "wlr/xwayland/xwayland.h"
#undef class
#undef static
}

namespace zenith::xwayland_input {

static const char* xwayland_title_for_surface(wlr_surface* surface) {
	if (surface == nullptr) {
		return "";
	}
	auto* xwayland_surface = wlr_xwayland_surface_try_from_wlr_surface(surface);
	if (xwayland_surface == nullptr || xwayland_surface->title == nullptr) {
		return "";
	}
	return xwayland_surface->title;
}

static bool is_xwayland_surface(wlr_surface* surface) {
	return surface != nullptr && wlr_xwayland_surface_try_from_wlr_surface(surface) != nullptr;
}

static void xwayland_xy_for_surface(wlr_surface* surface, int* out_x, int* out_y) {
	if (out_x == nullptr || out_y == nullptr) {
		return;
	}
	*out_x = 0;
	*out_y = 0;
	if (surface == nullptr) {
		return;
	}
	auto* xwayland_surface = wlr_xwayland_surface_try_from_wlr_surface(surface);
	if (xwayland_surface == nullptr) {
		return;
	}
	*out_x = xwayland_surface->x;
	*out_y = xwayland_surface->y;
}

bool remap_pointer_coords(wlr_surface* surface, double* x, double* y) {
	if (surface == nullptr || x == nullptr || y == nullptr) {
		return false;
	}
	auto* xwayland_surface = wlr_xwayland_surface_try_from_wlr_surface(surface);
	if (xwayland_surface == nullptr || xwayland_surface->override_redirect) {
		return false;
	}

	const pixman_box32_t extents = surface->input_region.extents;
	if (extents.x1 == 0 && extents.y1 == 0) {
		return false;
	}

	*x += extents.x1;
	*y += extents.y1;

	// Keep the pointer inside the client-reported region so wlroots keeps focus.
	if (extents.x2 > extents.x1) {
		*x = std::clamp(*x, (double) extents.x1, (double) extents.x2 - 1.0);
	}
	if (extents.y2 > extents.y1) {
		*y = std::clamp(*y, (double) extents.y1, (double) extents.y2 - 1.0);
	}
	return true;
}

void log_pointer_focus_debug(const char* phase,
                             ZenithServer* server,
                             size_t view_id,
                             wlr_surface* target_surface,
                             double x,
                             double y,
                             bool remapped,
                             bool fallback_used) {
	if (!zenith_xwayland_input_debug_enabled() || server == nullptr || server->seat == nullptr) {
		return;
	}
	wlr_surface* focused = server->seat->pointer_state.focused_surface;
	pixman_box32_t extents = {0, 0, 0, 0};
	if (target_surface != nullptr) {
		extents = target_surface->input_region.extents;
	}
	int target_xw_x = 0;
	int target_xw_y = 0;
	int focus_xw_x = 0;
	int focus_xw_y = 0;
	xwayland_xy_for_surface(target_surface, &target_xw_x, &target_xw_y);
	xwayland_xy_for_surface(focused, &focus_xw_x, &focus_xw_y);
	double root_x = x + (double) target_xw_x;
	double root_y = y + (double) target_xw_y;
	double cursor_x = 0.0;
	double cursor_y = 0.0;
	if (server->pointer != nullptr && server->pointer->cursor != nullptr) {
		cursor_x = server->pointer->cursor->x;
		cursor_y = server->pointer->cursor->y;
	}
	const int target_width = target_surface != nullptr ? target_surface->current.width : 0;
	const int target_height = target_surface != nullptr ? target_surface->current.height : 0;
	wlr_log(
		WLR_INFO,
		"zenith:xw-input phase=%s view=%zu xy=(%.2f,%.2f) root_xy=(%.2f,%.2f) cursor_xy=(%.2f,%.2f) remapped=%d fallback=%d target=%p focus=%p target_wh=%dx%d extents=[%d,%d,%d,%d] target_xw=%d focus_xw=%d target_xw_xy=(%d,%d) focus_xw_xy=(%d,%d) target_title=\"%s\" focus_title=\"%s\"",
		phase,
		view_id,
		x,
		y,
		root_x,
		root_y,
		cursor_x,
		cursor_y,
		remapped ? 1 : 0,
		fallback_used ? 1 : 0,
		(void*) target_surface,
		(void*) focused,
		target_width,
		target_height,
		extents.x1,
		extents.y1,
		extents.x2,
		extents.y2,
		is_xwayland_surface(target_surface) ? 1 : 0,
		is_xwayland_surface(focused) ? 1 : 0,
		target_xw_x,
		target_xw_y,
		focus_xw_x,
		focus_xw_y,
		xwayland_title_for_surface(target_surface),
		xwayland_title_for_surface(focused)
	);
}

void log_button_focus_debug(const char* phase,
                            ZenithServer* server,
                            int32_t linux_button,
                            bool is_pressed) {
	if (!zenith_xwayland_input_debug_enabled() || server == nullptr || server->seat == nullptr) {
		return;
	}
	wlr_surface* focused = server->seat->pointer_state.focused_surface;
	wlr_log(
		WLR_INFO,
		"zenith:xw-input phase=%s button=%d pressed=%d focus=%p focus_xw=%d focus_title=\"%s\"",
		phase,
		linux_button,
		is_pressed ? 1 : 0,
		(void*) focused,
		is_xwayland_surface(focused) ? 1 : 0,
		xwayland_title_for_surface(focused)
	);
}

void log_missing_surface_debug(size_t view_id, double x, double y) {
	if (!zenith_xwayland_input_debug_enabled()) {
		return;
	}
	wlr_log(WLR_INFO, "zenith:xw-input phase=missing-surface view=%zu xy=(%.2f,%.2f)", view_id, x, y);
}

void remap_global_configure_coords(ZenithServer* server,
                                   int global_x,
                                   int global_y,
                                   int* xwayland_x,
                                   int* xwayland_y,
                                   bool* remapped,
                                   int* output_origin_x,
                                   int* output_origin_y) {
	if (xwayland_x == nullptr || xwayland_y == nullptr) {
		return;
	}
	*xwayland_x = global_x;
	*xwayland_y = global_y;
	if (remapped != nullptr) {
		*remapped = false;
	}
	if (output_origin_x != nullptr) {
		*output_origin_x = 0;
	}
	if (output_origin_y != nullptr) {
		*output_origin_y = 0;
	}
	if (server == nullptr || server->output_layout == nullptr || server->output_manager == nullptr) {
		return;
	}
	if (server->output_manager->mode() != multimonitor::MultiMonitorMode::Extend) {
		return;
	}
	struct wlr_output* output = wlr_output_layout_output_at(
		server->output_layout,
		(double) global_x,
		(double) global_y
	);
	if (output == nullptr) {
		return;
	}
	struct wlr_box box = {};
	wlr_output_layout_get_box(server->output_layout, output, &box);
	*xwayland_x = global_x - box.x;
	*xwayland_y = global_y - box.y;
	if (output_origin_x != nullptr) {
		*output_origin_x = box.x;
	}
	if (output_origin_y != nullptr) {
		*output_origin_y = box.y;
	}
	if (remapped != nullptr) {
		*remapped = box.x != 0 || box.y != 0;
	}
}

bool configure_xwayland_toplevel(ZenithServer* server,
                                 size_t view_id,
                                 int width,
                                 int height,
                                 bool has_x,
                                 bool has_y,
                                 double requested_x,
                                 double requested_y) {
	if (server == nullptr) {
		return false;
	}

	auto xwayland_it = server->xwayland_toplevels.find(view_id);
	if (xwayland_it == server->xwayland_toplevels.end()) {
		return false;
	}

	auto* xwayland_toplevel = xwayland_it->second.get();
	auto* xwayland_surface = xwayland_toplevel->xwayland_surface;
	if (xwayland_surface == nullptr) {
		return true;
	}

	int x = xwayland_toplevel->has_configure_position ? xwayland_toplevel->configure_x : xwayland_surface->x;
	int y = xwayland_toplevel->has_configure_position ? xwayland_toplevel->configure_y : xwayland_surface->y;
	bool used_global_remap = false;
	int output_origin_x = 0;
	int output_origin_y = 0;
	if (has_x) {
		x = (int) std::lround(requested_x);
	}
	if (has_y) {
		y = (int) std::lround(requested_y);
	}
	if (has_x && has_y) {
		int remapped_x = x;
		int remapped_y = y;
		remap_global_configure_coords(
			server,
			x,
			y,
			&remapped_x,
			&remapped_y,
			&used_global_remap,
			&output_origin_x,
			&output_origin_y
		);
		x = remapped_x;
		y = remapped_y;
	}

	xwayland_toplevel->has_configure_position = true;
	xwayland_toplevel->configure_x = x;
	xwayland_toplevel->configure_y = y;
	wlr_xwayland_surface_configure(
		xwayland_surface,
		x,
		y,
		(uint16_t) width,
		(uint16_t) height
	);

	if (zenith_xwayland_input_debug_enabled()) {
		wlr_log(
			WLR_INFO,
			"zenith:xw-configure view=%zu req_xy=(%.2f,%.2f) has_xy=(%d,%d) send_xy=(%d,%d) send_wh=(%d,%d) has_cfg_xy=%d cfg_xy=(%d,%d) remapped_global=%d output_origin=(%d,%d) title=\"%s\"",
			view_id,
			requested_x,
			requested_y,
			has_x ? 1 : 0,
			has_y ? 1 : 0,
			x,
			y,
			width,
			height,
			xwayland_toplevel->has_configure_position ? 1 : 0,
			xwayland_toplevel->configure_x,
			xwayland_toplevel->configure_y,
			used_global_remap ? 1 : 0,
			output_origin_x,
			output_origin_y,
			xwayland_surface->title != nullptr ? xwayland_surface->title : ""
		);
	}

	return true;
}

} // namespace zenith::xwayland_input
