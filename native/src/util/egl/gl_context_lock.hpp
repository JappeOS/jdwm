#pragma once

#include <mutex>

namespace zenith::egl {

std::recursive_mutex& gl_context_mutex();
void lock_gl_context();
void unlock_gl_context();
bool try_lock_gl_context();

class GlContextGuard {
public:
	GlContextGuard();
	~GlContextGuard();

	GlContextGuard(const GlContextGuard&) = delete;
	GlContextGuard& operator=(const GlContextGuard&) = delete;
};

class TryGlContextGuard {
public:
	TryGlContextGuard();
	~TryGlContextGuard();

	TryGlContextGuard(const TryGlContextGuard&) = delete;
	TryGlContextGuard& operator=(const TryGlContextGuard&) = delete;

	bool owns_lock() const;

private:
	bool locked_ = false;
};

} // namespace zenith::egl
