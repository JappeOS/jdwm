#pragma once
#include <cstdlib>
#include <strings.h>

namespace zenith::config {
inline bool flag_value(const char* value) {
	return value &&
			*value &&
			strcasecmp(value, "0") &&
			strcasecmp(value, "false") &&
			strcasecmp(value, "off") &&
			strcasecmp(value, "no");
}

inline bool flag_enabled(const char* name) {
	return flag_value(std::getenv(name));
}
}
