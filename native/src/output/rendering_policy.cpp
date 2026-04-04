#include "rendering_policy.hpp"

#include <cstdlib>

#include "server.hpp"
#include "zenith_output_manager.hpp"

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

bool env_is_enabled(const char* name) {
	const char* value = std::getenv(name);
	if (value == nullptr || value[0] == '\0') {
		return false;
	}
	return value[0] != '0';
}

} // namespace

namespace zenith::render {

size_t desired_swapchain_buffer_count(const ZenithServer* server) {
	const size_t override_value = parse_swapchain_buffer_count_override();
	if (override_value != 0) {
		return override_value;
	}
	if (server != nullptr && server->output_manager != nullptr &&
	    server->output_manager->mode() == multimonitor::MultiMonitorMode::Extend) {
		// Extended desktop can be presented by multiple outputs with different
		// refresh rates. Keep a deeper history to avoid reusing a buffer that is
		// still being scanned out on another output/GPU.
		return 8;
	}
	return 4;
}

bool force_software_cursor() {
	return env_is_enabled("ZENITH_FORCE_SOFTWARE_CURSOR");
}

bool allow_direct_scanout() {
	return env_is_enabled("ZENITH_ALLOW_DIRECT_SCANOUT");
}

} // namespace zenith::render
