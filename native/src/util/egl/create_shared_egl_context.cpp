#include "create_shared_egl_context.hpp"

#include <cstring>
#include <iostream>
#include <vector>

extern "C" {
#include <wlr/render/egl.h>
#include <EGL/eglext.h>
}

static const EGLint config_attribs[] = {
	  EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
	  EGL_RED_SIZE, 8,
	  EGL_GREEN_SIZE, 8,
	  EGL_BLUE_SIZE, 8,
	  EGL_ALPHA_SIZE, 8,
	  EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	  EGL_NONE,
};

struct wlr_egl* create_shared_egl_context(struct wlr_egl* egl) {
	EGLDisplay display = wlr_egl_get_display(egl);
	EGLContext context = wlr_egl_get_context(egl);

	EGLConfig egl_config;
	EGLint matched = 0;
	if (!eglChooseConfig(display, config_attribs, &egl_config, 1, &matched)) {
		std::cerr << "eglChooseConfig failed" << std::endl;
		return nullptr;
	}
	if (matched == 0) {
		std::cerr << "Failed to match an EGL config" << std::endl;
		return nullptr;
	}

	// Build context attributes dynamically. If the display supports
	// EGL_EXT_create_context_robustness, wlroots creates its main context with
	// EGL_LOSE_CONTEXT_ON_RESET_EXT. Mesa 25+ (AMD) enforces that shared
	// contexts must use the same reset notification strategy, so we must match.
	std::vector<EGLint> attribs = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_HIGH_IMG,
	};

	const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
	if (extensions && strstr(extensions, "EGL_EXT_create_context_robustness")) {
		attribs.push_back(EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT);
		attribs.push_back(EGL_LOSE_CONTEXT_ON_RESET_EXT);
	}

	attribs.push_back(EGL_NONE);

	EGLContext shared_egl_context = eglCreateContext(display, egl_config, context, attribs.data());
	if (shared_egl_context == EGL_NO_CONTEXT) {
		std::cerr << "Failed to create EGL context" << std::endl;
		return nullptr;
	}

	return wlr_egl_create_with_context(display, shared_egl_context);
}
