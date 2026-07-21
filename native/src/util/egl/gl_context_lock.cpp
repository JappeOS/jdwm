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

bool try_lock_gl_context() {
	return gl_context_mutex().try_lock();
}

GlContextGuard::GlContextGuard() {
	lock_gl_context();
}

GlContextGuard::~GlContextGuard() {
	unlock_gl_context();
}

TryGlContextGuard::TryGlContextGuard()
	: locked_(try_lock_gl_context()) {
}

TryGlContextGuard::~TryGlContextGuard() {
	if (locked_) {
		unlock_gl_context();
	}
}

bool TryGlContextGuard::owns_lock() const {
	return locked_;
}

} // namespace zenith::egl
