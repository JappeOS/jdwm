#pragma once

#include <cstddef>

struct ZenithServer;

namespace zenith::render {

size_t desired_swapchain_buffer_count(const ZenithServer* server);
bool force_software_cursor();
bool allow_direct_scanout();

} // namespace zenith::render
