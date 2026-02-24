#pragma once

namespace multimonitor {

enum class MultiMonitorMode {
	Off,
	Last,
	Extend,
};

MultiMonitorMode parse_multi_monitor_mode_from_env();
const char* to_string(MultiMonitorMode mode);
bool is_enabled(MultiMonitorMode mode);

} // namespace multimonitor
