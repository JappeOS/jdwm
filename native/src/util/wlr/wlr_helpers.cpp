#include "wlr_helpers.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <GLES2/gl2ext.h>
#include <libdrm/drm_fourcc.h>

extern "C" {
#define static
#include <wlr/util/log.h>
#include <wlr/render/drm_format_set.h>
#include <wlr/render/egl.h>
#include <wlr/render/allocator.h>
#include <wlr/render/interface.h>
#undef static
// Private wlroots headers for internal struct access
#include <render/egl.h>
#include <render/gles2.h>
#include <render/wlr_renderer.h>
}

struct wlr_drm_format* get_output_format(wlr_output* output) {
	struct wlr_allocator* allocator = output->allocator;
	assert(allocator != nullptr);

	const struct wlr_drm_format_set* display_formats =
		  wlr_output_get_primary_formats(output, allocator->buffer_caps);
	struct wlr_drm_format* format = output_pick_format(output, display_formats,
	                                                   output->render_format);
	if (format == nullptr) {
		wlr_log(WLR_ERROR, "Failed to pick primary buffer format for output '%s'",
		        output->name);
		return nullptr;
	}
	return format;
}

struct wlr_drm_format*
output_pick_format(struct wlr_output* output, const struct wlr_drm_format_set* display_formats, uint32_t fmt) {
	struct wlr_renderer* renderer = output->renderer;
	struct wlr_allocator* allocator = output->allocator;
	assert(renderer != nullptr && allocator != nullptr);

	const struct wlr_drm_format_set* render_formats =
		  wlr_renderer_get_render_formats(renderer);
	if (render_formats == nullptr) {
		wlr_log(WLR_ERROR, "Failed to get render formats");
		return nullptr;
	}

	const struct wlr_drm_format* render_format =
		  wlr_drm_format_set_get(render_formats, fmt);
	if (render_format == nullptr) {
		wlr_log(WLR_DEBUG, "Renderer doesn't support format 0x%" PRIX32, fmt);
		return nullptr;
	}

	struct wlr_drm_format* format = nullptr;
	if (display_formats != nullptr) {
		const struct wlr_drm_format* display_format =
			  wlr_drm_format_set_get(display_formats, fmt);
		if (display_format == nullptr) {
			wlr_log(WLR_DEBUG, "Output doesn't support format 0x%" PRIX32, fmt);
			return nullptr;
		}
		format = wlr_drm_format_intersect(display_format, render_format);
	} else {
		// The output can display any format
		format = wlr_drm_format_dup(render_format);
	}

	if (format == nullptr) {
		wlr_log(WLR_DEBUG, "Failed to intersect display and render "
		                   "modifiers for format 0x%" PRIX32 " on output '%s",
		        fmt, output->name);
		return nullptr;
	}

	return format;
}

// wlr_renderer_get_render_formats is now provided by the private header render/wlr_renderer.h

struct wlr_drm_format* wlr_drm_format_intersect(const struct wlr_drm_format* a, const struct wlr_drm_format* b) {
	assert(a->format == b->format);

	size_t format_cap = a->len < b->len ? a->len : b->len;
	auto* format = static_cast<wlr_drm_format*>(calloc(1, sizeof(struct wlr_drm_format)));
	if (format == nullptr) {
		wlr_log_errno(WLR_ERROR, "Allocation failed");
		return nullptr;
	}
	format->format = a->format;
	format->capacity = format_cap;
	format->modifiers = static_cast<uint64_t*>(calloc(format_cap, sizeof(uint64_t)));
	if (format->modifiers == nullptr) {
		free(format);
		return nullptr;
	}

	for (size_t i = 0; i < a->len; i++) {
		for (size_t j = 0; j < b->len; j++) {
			if (a->modifiers[i] == b->modifiers[j]) {
				assert(format->len < format->capacity);
				format->modifiers[format->len] = a->modifiers[i];
				format->len++;
				break;
			}
		}
	}

	// If the intersection is empty, then the formats aren't compatible with
	// each other.
	if (format->len == 0) {
		free(format->modifiers);
		free(format);
		return nullptr;
	}

	return format;
}

struct wlr_drm_format* wlr_drm_format_dup(const struct wlr_drm_format* format) {
	assert(format->len <= format->capacity);
	auto* duped_format = static_cast<wlr_drm_format*>(calloc(1, sizeof(struct wlr_drm_format)));
	if (duped_format == nullptr) {
		return nullptr;
	}
	duped_format->format = format->format;
	duped_format->len = format->len;
	duped_format->capacity = format->capacity;
	duped_format->modifiers = static_cast<uint64_t*>(calloc(format->capacity, sizeof(uint64_t)));
	if (duped_format->modifiers == nullptr) {
		free(duped_format);
		return nullptr;
	}
	memcpy(duped_format->modifiers, format->modifiers, format->len * sizeof(uint64_t));
	return duped_format;
}

// wlr_gles2_renderer, wlr_egl_context, wlr_egl struct definitions,
// push/pop_gles2_debug, wlr_egl_save/restore_context, wlr_egl_make_current
// are all provided by the private wlroots headers included above.


static void destroy_buffer(struct wlr_gles2_buffer* buffer) {
	wl_list_remove(&buffer->link);
	wlr_addon_finish(&buffer->addon);

	struct wlr_egl_context prev_ctx;
	wlr_egl_make_current(buffer->renderer->egl, &prev_ctx);

	glDeleteFramebuffers(1, &buffer->fbo);
	glDeleteRenderbuffers(1, &buffer->rbo);

	wlr_egl_destroy_image(buffer->renderer->egl, buffer->image);

	wlr_egl_restore_context(&prev_ctx);

	free(buffer);
}

static void handle_buffer_destroy(struct wlr_addon* addon) {
	struct wlr_gles2_buffer* buffer =
		  wl_container_of(addon, buffer, addon);
	destroy_buffer(buffer);
}

static const struct wlr_addon_interface buffer_addon_impl = {
	  .name = "wlr_gles2_buffer",
	  .destroy = handle_buffer_destroy,
};

struct wlr_gles2_buffer* create_buffer(struct wlr_gles2_renderer* renderer, struct wlr_buffer* wlr_buffer) {
	wlr_gles2_buffer* buffer = static_cast<wlr_gles2_buffer*>(calloc(1, sizeof(*buffer)));
	if (buffer == nullptr) {
		wlr_log_errno(WLR_ERROR, "Allocation failed");
		return nullptr;
	}
	buffer->buffer = wlr_buffer;
	buffer->renderer = renderer;

	struct wlr_dmabuf_attributes dmabuf = {};
	if (!wlr_buffer_get_dmabuf(wlr_buffer, &dmabuf)) {
		free(buffer);
		return nullptr;
	}

	bool external_only;
	buffer->image = wlr_egl_create_image_from_dmabuf(renderer->egl,
	                                                 &dmabuf, &external_only);
	if (buffer->image == EGL_NO_IMAGE_KHR) {
		free(buffer);
		return nullptr;
	}

	glGenRenderbuffers(1, &buffer->rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, buffer->rbo);
	renderer->procs.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER,
	                                                       buffer->image);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &buffer->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                          GL_RENDERBUFFER, buffer->rbo);
	GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
		wlr_log(WLR_ERROR, "Failed to create FBO");
		goto error_image;
	}

	wlr_addon_init(&buffer->addon, &wlr_buffer->addons, renderer,
	               &buffer_addon_impl);

	wl_list_insert(&renderer->buffers, &buffer->link);

	wlr_log(WLR_DEBUG, "Created GL FBO for buffer %dx%d",
	        wlr_buffer->width, wlr_buffer->height);

	return buffer;

	error_image:
	wlr_egl_destroy_image(renderer->egl, buffer->image);
	free(buffer);
	return nullptr;
}

// wlr_egl_create_image_from_dmabuf and wlr_egl_destroy_image
// are provided by the wlroots library (declared in render/egl.h).
