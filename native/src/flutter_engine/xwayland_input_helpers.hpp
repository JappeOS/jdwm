#pragma once

#include <cstddef>
#include <cstdint>

struct ZenithServer;
struct wlr_surface;

namespace zenith::xwayland_input {

bool remap_pointer_coords(wlr_surface* surface, double* x, double* y);

void log_pointer_focus_debug(const char* phase,
                             ZenithServer* server,
                             size_t view_id,
                             wlr_surface* target_surface,
                             double x,
                             double y,
                             bool remapped,
                             bool fallback_used);

void log_button_focus_debug(const char* phase,
                            ZenithServer* server,
                            int32_t linux_button,
                            bool is_pressed);

void log_missing_surface_debug(size_t view_id, double x, double y);

void remap_global_configure_coords(ZenithServer* server,
                                   int global_x,
                                   int global_y,
                                   int* xwayland_x,
                                   int* xwayland_y,
                                   bool* remapped,
                                   int* output_origin_x,
                                   int* output_origin_y);

bool configure_xwayland_toplevel(ZenithServer* server,
                                 size_t view_id,
                                 int width,
                                 int height,
                                 bool has_x,
                                 bool has_y,
                                 double requested_x,
                                 double requested_y);

} // namespace zenith::xwayland_input
