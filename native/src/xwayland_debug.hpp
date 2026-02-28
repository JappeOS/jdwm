#pragma once

#include <cstdlib>

inline bool zenith_xwayland_input_debug_enabled() {
	static int enabled = -1;
	if (enabled == -1) {
		const char* value = getenv("ZENITH_XWAYLAND_INPUT_DEBUG");
		enabled = value != nullptr && value[0] != '\0' && value[0] != '0' ? 1 : 0;
	}
	return enabled == 1;
}
