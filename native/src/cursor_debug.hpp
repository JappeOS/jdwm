#pragma once

#include "util/environment.hpp"

inline bool zenith_cursor_debug_enabled() {
	static const bool enabled = zenith::config::flag_enabled("ZENITH_CURSOR_DEBUG");
	return enabled;
}
