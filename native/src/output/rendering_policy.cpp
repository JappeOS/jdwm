#include <cstdlib>

#include "server.hpp"
#include "zenith_output_manager.hpp"
#include "util/environment.hpp"
#include "rendering_policy.hpp"

namespace {

size_t parse_swapchain_buffer_count_override() {
	const char* value = std::getenv("ZENITH_SWAPCHAIN_BUFFERS");
	if (value == nullptr || value[0] == '\0') {
		return 0;
	}
	char* end = nullptr;
	unsigned long parsed = std::strtoul(value, &end, 10);
	if (end == value || (end != nullptr && *end != '\0')) {
		return 0;
	}
	if (parsed < 4) {
		return 4;
	}
	return static_cast<size_t>(parsed);
}

} // namespace

namespace zenith::render {

size_t desired_swapchain_buffer_count(const ZenithServer* server) {
	const size_t override_value = parse_swapchain_buffer_count_override();
	if (override_value != 0) {
		return override_value;
	}
	// Multiple outputs can retain frames concurrently, including across GPUs.
	return 8;
}

bool force_software_cursor() {
	return zenith::config::flag_enabled("ZENITH_FORCE_SOFTWARE_CURSOR");
}

bool allow_direct_scanout() {
	return zenith::config::flag_enabled("ZENITH_ALLOW_DIRECT_SCANOUT");
}

} // namespace zenith::render
