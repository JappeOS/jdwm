#pragma once

#include <mutex>

namespace zenith::egl {

std::recursive_mutex& gl_context_mutex();
void lock_gl_context();
void unlock_gl_context();

class GlContextGuard {
public:
	GlContextGuard();
	~GlContextGuard();

	GlContextGuard(const GlContextGuard&) = delete;
	GlContextGuard& operator=(const GlContextGuard&) = delete;
};

} // namespace zenith::egl
