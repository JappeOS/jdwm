#include "gl_context_lock.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <time.h>

namespace zenith::egl {

static constexpr uint64_t FLUTTER_FRAME_RENDER_TIMEOUT_NS = 50'000'000ull;

static uint64_t monotonic_now_ns() {
	timespec now{};
	clock_gettime(CLOCK_MONOTONIC, &now);
	return static_cast<uint64_t>(now.tv_sec) * 1'000'000'000ull + static_cast<uint64_t>(now.tv_nsec);
}

static std::atomic_bool& gl_context_serialization_enabled_state() {
	static std::atomic_bool enabled{false};
	return enabled;
}

static std::atomic_bool& flutter_frame_rendering_active_state() {
	static std::atomic_bool active{false};
	return active;
}

static std::atomic<uint64_t>& flutter_frame_rendering_active_since_ns_state() {
	static std::atomic<uint64_t> active_since_ns{0};
	return active_since_ns;
}

void set_gl_context_serialization_enabled(bool enabled) {
	gl_context_serialization_enabled_state().store(enabled);
}

bool gl_context_serialization_enabled() {
	return gl_context_serialization_enabled_state().load();
}

void set_flutter_frame_rendering_active(bool active) {
	flutter_frame_rendering_active_since_ns_state().store(
		active ? monotonic_now_ns() : 0,
		std::memory_order_release
	);
	flutter_frame_rendering_active_state().store(active, std::memory_order_release);
}

static bool flutter_frame_rendering_active() {
	return flutter_frame_rendering_active_state().load(std::memory_order_acquire);
}

bool should_defer_for_flutter_frame_rendering() {
	if (!gl_context_serialization_enabled() || !flutter_frame_rendering_active()) {
		return false;
	}

	uint64_t active_since_ns =
		flutter_frame_rendering_active_since_ns_state().load(std::memory_order_acquire);
	uint64_t now_ns = monotonic_now_ns();
	if (active_since_ns != 0 &&
	    now_ns > active_since_ns &&
	    now_ns - active_since_ns > FLUTTER_FRAME_RENDER_TIMEOUT_NS) {
		flutter_frame_rendering_active_state().store(false, std::memory_order_release);
		flutter_frame_rendering_active_since_ns_state().store(0, std::memory_order_release);
		return false;
	}
	return true;
}

static std::recursive_mutex& gl_context_mutex() {
	static std::recursive_mutex mutex;
	return mutex;
}

static bool lock_gl_context() {
	if (!gl_context_serialization_enabled()) {
		return false;
	}
	gl_context_mutex().lock();
	return true;
}

static void unlock_gl_context() {
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
