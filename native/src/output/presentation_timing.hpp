#pragma once

#include <cstdint>

struct wlr_output;

namespace zenith::render {

double output_refresh_hz(const wlr_output* output);
uint64_t next_presentation_time_ns(uint64_t now_ns, const wlr_output* output);

} // namespace zenith::render
