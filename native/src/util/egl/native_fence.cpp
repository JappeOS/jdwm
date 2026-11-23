#include "native_fence.hpp"
#include "egl_extensions.hpp"
#include "util/fence_diagnostics.hpp"
#include <GLES2/gl2.h>

int export_native_fence(EGLDisplay display, EGLSyncKHR* retained_sync) {
	if (retained_sync)
		*retained_sync = EGL_NO_SYNC_KHR;

	if (!eglCreateSyncKHR || !eglDestroySyncKHR || !eglDupNativeFenceFDANDROID)
		return -1;

	const EGLint attributes[] = {EGL_NONE};
	auto sync = eglCreateSyncKHR(display, EGL_SYNC_NATIVE_FENCE_ANDROID, attributes);
	if (sync == EGL_NO_SYNC_KHR) {
		static thread_local zenith::diagnostics::FenceFailures failures;
		failures.record("egl-create-sync", 0, eglGetError());
		return -1;
	}

	glFlush();

	int fd = eglDupNativeFenceFDANDROID(display, sync);
	if (fd < 0) {
		static thread_local zenith::diagnostics::FenceFailures failures;
		failures.record("egl-dup-fence", 0, eglGetError());
	}

	if (fd >= 0 && retained_sync)
		*retained_sync = sync;
	else
		eglDestroySyncKHR(display, sync);
	
	return fd;
}
