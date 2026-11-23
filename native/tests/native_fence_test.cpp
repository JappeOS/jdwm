#include "util/egl/egl_extensions.hpp"
#include "util/egl/native_fence.hpp"
#include <cassert>
#include "util/duplicate_fd.hpp"
#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
	auto token = reinterpret_cast<EGLSyncKHR>(1);
	EGLSyncKHR retained = token;
	assert(export_native_fence(EGL_NO_DISPLAY, &retained) == -1 && !retained);
	static int destroyed = 0;
	eglCreateSyncKHR = [](EGLDisplay, EGLenum, const EGLint*) { return reinterpret_cast<EGLSyncKHR>(1); };
	eglDestroySyncKHR = [](EGLDisplay, EGLSyncKHR) -> EGLBoolean { ++destroyed; return EGL_TRUE; };
	eglDupNativeFenceFDANDROID = [](EGLDisplay, EGLSyncKHR) { return -1; };
	assert(export_native_fence(EGL_NO_DISPLAY, &retained) == -1 && !retained && destroyed == 1);
	eglDupNativeFenceFDANDROID = [](EGLDisplay, EGLSyncKHR) { return eventfd(0, EFD_CLOEXEC); };
	int fd = export_native_fence(EGL_NO_DISPLAY, &retained);
	assert(fd >= 0 && retained == token && destroyed == 1);
	int copy = zenith::duplicate_cloexec(fd);
	assert(copy >= 0 && (fcntl(copy, F_GETFD) & FD_CLOEXEC));
	close(copy);
	assert(zenith::duplicate_cloexec(-1) == -1 && errno == EBADF);
	close(fd);
	eglDestroySyncKHR(EGL_NO_DISPLAY, retained);
	fd = export_native_fence(EGL_NO_DISPLAY);
	assert(fd >= 0 && destroyed == 3);
	close(fd);
}
