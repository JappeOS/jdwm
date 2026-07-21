#include "gl_context_lock.hpp"

#include <atomic>

namespace zenith::egl {

static std::atomic_bool& gl_context_serialization_enabled_state() {
	static std::atomic_bool enabled{false};
	return enabled;
}

static std::atomic_bool& flutter_frame_rendering_active_state() {
	static std::atomic_bool active{false};
	return active;
}

void set_gl_context_serialization_enabled(bool enabled) {
	gl_context_serialization_enabled_state().store(enabled);
}

bool gl_context_serialization_enabled() {
	return gl_context_serialization_enabled_state().load();
}

void set_flutter_frame_rendering_active(bool active) {
	flutter_frame_rendering_active_state().store(active, std::memory_order_release);
}

bool flutter_frame_rendering_active() {
	return flutter_frame_rendering_active_state().load(std::memory_order_acquire);
}

std::recursive_mutex& gl_context_mutex() {
	static std::recursive_mutex mutex;
	return mutex;
}

bool lock_gl_context() {
	if (!gl_context_serialization_enabled()) {
		return false;
	}
	gl_context_mutex().lock();
	return true;
}

void unlock_gl_context() {
	gl_context_mutex().unlock();
}

GlContextGuard::GlContextGuard()
	: locked_(lock_gl_context()) {
}

GlContextGuard::~GlContextGuard() {
	if (locked_) {
		unlock_gl_context();
	}
}

TryGlContextGuard::TryGlContextGuard() {
	if (!gl_context_serialization_enabled()) {
		owns_lock_ = true;
		return;
	}

	locked_mutex_ = gl_context_mutex().try_lock();
	owns_lock_ = locked_mutex_;
}

TryGlContextGuard::~TryGlContextGuard() {
	if (locked_mutex_) {
		unlock_gl_context();
	}
}

bool TryGlContextGuard::owns_lock() const {
	return owns_lock_;
}

} // namespace zenith::egl
