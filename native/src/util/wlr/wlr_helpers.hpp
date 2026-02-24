#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <wayland-util.h>

extern "C" {
#include <wlr/types/wlr_output.h>
#define static
#include <wlr/util/addon.h>
#undef static
// Private wlroots headers for internal struct definitions
#include <render/egl.h>
#include <render/gles2.h>
}

struct wlr_drm_format* get_output_format(wlr_output* output);

struct wlr_drm_format* output_pick_format(struct wlr_output* output,
                                          const struct wlr_drm_format_set* display_formats,
                                          uint32_t fmt);

struct wlr_drm_format* wlr_drm_format_intersect(
	  const struct wlr_drm_format* a, const struct wlr_drm_format* b);

struct wlr_drm_format* wlr_drm_format_dup(const struct wlr_drm_format* format);

struct wlr_gles2_buffer* create_buffer(struct wlr_gles2_renderer* renderer,
                                       struct wlr_buffer* wlr_buffer);
