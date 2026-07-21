#include "create_shared_egl_context.hpp"

#include <cstring>
#include <vector>

extern "C" {
#include <wlr/render/egl.h>
#include <wlr/util/log.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
}

static const EGLint pbuffer_config_attribs[] = {
	  EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
	  EGL_RED_SIZE, 8,
	  EGL_GREEN_SIZE, 8,
	  EGL_BLUE_SIZE, 8,
	  EGL_ALPHA_SIZE, 8,
	  EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	  EGL_NONE,
};

static const EGLint render_config_attribs[] = {
	  EGL_RED_SIZE, 8,
	  EGL_GREEN_SIZE, 8,
	  EGL_BLUE_SIZE, 8,
	  EGL_ALPHA_SIZE, 8,
	  EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	  EGL_NONE,
};

static bool extension_supported(const char* extensions, const char* extension) {
	if (extensions == nullptr || extension == nullptr || extension[0] == '\0') {
		return false;
	}
	const size_t extension_len = strlen(extension);
	const char* cursor = extensions;
	while ((cursor = strstr(cursor, extension)) != nullptr) {
		const bool starts_token = cursor == extensions || cursor[-1] == ' ';
		const bool ends_token = cursor[extension_len] == '\0' || cursor[extension_len] == ' ';
		if (starts_token && ends_token) {
			return true;
		}
		cursor += extension_len;
	}
	return false;
}

struct ContextConfig {
	EGLConfig config = nullptr;
	bool valid = false;
};

static ContextConfig choose_context_config(EGLDisplay display, const char* extensions) {
	EGLConfig egl_config = nullptr;
	EGLint matched = 0;
	if (eglChooseConfig(display, pbuffer_config_attribs, &egl_config, 1, &matched) && matched > 0) {
		return {
			.config = egl_config,
			.valid = true,
		};
	}
	if (eglChooseConfig(display, render_config_attribs, &egl_config, 1, &matched) && matched > 0) {
		wlr_log(WLR_INFO, "zenith: using non-pbuffer EGL config for shared Flutter context");
		return {
			.config = egl_config,
			.valid = true,
		};
	}
	if (extension_supported(extensions, "EGL_KHR_no_config_context") ||
	    extension_supported(extensions, "EGL_MESA_configless_context")) {
		wlr_log(WLR_INFO, "zenith: using configless EGL context for shared Flutter context");
#ifdef EGL_NO_CONFIG_KHR
		return {
			.config = EGL_NO_CONFIG_KHR,
			.valid = true,
		};
#else
		return {};
#endif
	}
	return {};
}

struct wlr_egl* create_shared_egl_context(struct wlr_egl* egl) {
	if (egl == nullptr) {
		return nullptr;
	}
	EGLDisplay display = wlr_egl_get_display(egl);
	EGLContext context = wlr_egl_get_context(egl);
	const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
	ContextConfig egl_config = choose_context_config(display, extensions);
	if (!egl_config.valid) {
		wlr_log(WLR_ERROR, "zenith: failed to match an EGL config for shared Flutter context");
		return nullptr;
	}

	// Build context attributes dynamically. If the display supports
	// EGL_EXT_create_context_robustness, wlroots creates its main context with
	// EGL_LOSE_CONTEXT_ON_RESET_EXT. Mesa 25+ (AMD) enforces that shared
	// contexts must use the same reset notification strategy, so we must match.
	std::vector<EGLint> attribs = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
	};
	if (extension_supported(extensions, "EGL_IMG_context_priority")) {
		attribs.push_back(EGL_CONTEXT_PRIORITY_LEVEL_IMG);
		attribs.push_back(EGL_CONTEXT_PRIORITY_HIGH_IMG);
	}

	if (extension_supported(extensions, "EGL_EXT_create_context_robustness")) {
		attribs.push_back(EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT);
		attribs.push_back(EGL_LOSE_CONTEXT_ON_RESET_EXT);
	}

	attribs.push_back(EGL_NONE);

	EGLContext shared_egl_context = eglCreateContext(display, egl_config.config, context, attribs.data());
	if (shared_egl_context == EGL_NO_CONTEXT) {
		wlr_log(WLR_ERROR, "zenith: failed to create shared Flutter EGL context");
		return nullptr;
	}

	return wlr_egl_create_with_context(display, shared_egl_context);
}
