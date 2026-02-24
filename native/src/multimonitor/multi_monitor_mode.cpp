#include "multi_monitor_mode.hpp"

#include <cstdlib>
#include <cstring>

namespace multimonitor {

static bool equals_ignore_case(const char* value, const char* expected) {
	if (value == nullptr || expected == nullptr) {
		return false;
	}
	while (*value != '\0' && *expected != '\0') {
		char a = *value;
		char b = *expected;
		if (a >= 'A' && a <= 'Z') {
			a = (char)(a - 'A' + 'a');
		}
		if (b >= 'A' && b <= 'Z') {
			b = (char)(b - 'A' + 'a');
		}
		if (a != b) {
			return false;
		}
		++value;
		++expected;
	}
	return *value == '\0' && *expected == '\0';
}

MultiMonitorMode parse_multi_monitor_mode_from_env() {
	const char* value = std::getenv("ZENITH_MULTI_MONITOR_MODE");
	if (value != nullptr && value[0] != '\0') {
		if (equals_ignore_case(value, "last")) {
			return MultiMonitorMode::Last;
		}
		if (equals_ignore_case(value, "extend")) {
			return MultiMonitorMode::Extend;
		}
		if (equals_ignore_case(value, "off")) {
			return MultiMonitorMode::Off;
		}
		if (equals_ignore_case(value, "1") || equals_ignore_case(value, "true") || equals_ignore_case(value, "on")) {
			return MultiMonitorMode::Extend;
		}
		return MultiMonitorMode::Off;
	}

	const char* legacy = std::getenv("ZENITH_MULTI_MONITOR");
	if (legacy != nullptr && legacy[0] != '\0') {
		if (equals_ignore_case(legacy, "1") || equals_ignore_case(legacy, "true") ||
		    equals_ignore_case(legacy, "on") || equals_ignore_case(legacy, "extend")) {
			return MultiMonitorMode::Extend;
		}
		if (equals_ignore_case(legacy, "last")) {
			return MultiMonitorMode::Last;
		}
	}
	return MultiMonitorMode::Off;
}

const char* to_string(MultiMonitorMode mode) {
	switch (mode) {
		case MultiMonitorMode::Off:
			return "off";
		case MultiMonitorMode::Last:
			return "last";
		case MultiMonitorMode::Extend:
			return "extend";
	}
	return "off";
}

bool is_enabled(MultiMonitorMode mode) {
	return mode != MultiMonitorMode::Off;
}

} // namespace multimonitor
