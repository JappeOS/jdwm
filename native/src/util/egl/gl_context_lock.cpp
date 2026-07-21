#include "gl_context_lock.hpp"

namespace zenith::egl {

std::recursive_mutex& gl_context_mutex() {
	static std::recursive_mutex mutex;
	return mutex;
}

void lock_gl_context() {
	gl_context_mutex().lock();
}

void unlock_gl_context() {
	gl_context_mutex().unlock();
}

GlContextGuard::GlContextGuard() {
	lock_gl_context();
}

GlContextGuard::~GlContextGuard() {
	unlock_gl_context();
}

} // namespace zenith::egl
