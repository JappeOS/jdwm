#pragma once
#include <EGL/egl.h>
#include <EGL/eglext.h>

// Requires a current GL context. On success the caller owns the fd and any requested sync.
int export_native_fence(EGLDisplay display, EGLSyncKHR* retained_sync = nullptr);
