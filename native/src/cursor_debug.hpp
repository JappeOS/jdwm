#pragma once

#include <cstdlib>

inline bool zenith_cursor_debug_enabled() {
	static int enabled = -1;
	if (enabled == -1) {
		const char* value = getenv("ZENITH_CURSOR_DEBUG");
		enabled = value != nullptr && value[0] != '\0' && value[0] != '0' ? 1 : 0;
	}
	return enabled == 1;
}
