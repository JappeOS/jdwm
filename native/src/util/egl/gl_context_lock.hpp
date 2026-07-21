#pragma once

#include <mutex>

namespace zenith::egl {

void set_gl_context_serialization_enabled(bool enabled);
bool gl_context_serialization_enabled();
void set_flutter_frame_rendering_active(bool active);
bool flutter_frame_rendering_active();
std::recursive_mutex& gl_context_mutex();
bool lock_gl_context();
void unlock_gl_context();

class GlContextGuard {
public:
	GlContextGuard();
	~GlContextGuard();

	GlContextGuard(const GlContextGuard&) = delete;
	GlContextGuard& operator=(const GlContextGuard&) = delete;

private:
	bool locked_ = false;
};

class TryGlContextGuard {
public:
	TryGlContextGuard();
	~TryGlContextGuard();

	TryGlContextGuard(const TryGlContextGuard&) = delete;
	TryGlContextGuard& operator=(const TryGlContextGuard&) = delete;

	bool owns_lock() const;

private:
	bool owns_lock_ = false;
	bool locked_mutex_ = false;
};

} // namespace zenith::egl
