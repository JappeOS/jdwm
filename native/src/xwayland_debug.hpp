#pragma once

#include "util/environment.hpp"

inline bool zenith_xwayland_input_debug_enabled() {
	static const bool enabled = zenith::config::flag_enabled("ZENITH_XWAYLAND_INPUT_DEBUG");
	return enabled;
}
