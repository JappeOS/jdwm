#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>
#include <time.h>
#include <unordered_map>

extern "C" {
#include <wlr/util/log.h>
}

namespace zenith::render {

inline bool frame_timing_enabled() {
	static const bool enabled = [] {
		const char* value = std::getenv("ZENITH_TRACE_FRAME_TIMING");
		return value != nullptr && value[0] != '\0' && value[0] != '0';
	}();
	return enabled;
}

inline uint64_t frame_trace_now() {
	if (!frame_timing_enabled()) {
		return 0;
	}
	timespec now{};
	clock_gettime(CLOCK_MONOTONIC, &now);
	return uint64_t(now.tv_sec) * 1'000'000'000ull + uint64_t(now.tv_nsec);
}

// Aggregate per thread and output: no global lock, GL queries, or per-frame
// log traffic. A sample is an attempt, not necessarily a presented frame.
inline void trace_frame_sample(const char* stage, const char* output,
                              uint64_t elapsed_ns, bool blocked = false) {
	if (!frame_timing_enabled()) {
		return;
	}
	struct Stats {
		uint64_t since = 0, count = 0, total = 0, maximum = 0, blocked = 0;
	};
	thread_local std::unordered_map<std::string, Stats> stats;
	auto& sample = stats[std::string(output) + ":" + stage];
	uint64_t now = frame_trace_now();
	if (sample.since == 0) sample.since = now;
	++sample.count;
	sample.total += elapsed_ns;
	if (elapsed_ns > sample.maximum) sample.maximum = elapsed_ns;
	if (blocked) ++sample.blocked;
	if (now - sample.since < 1'000'000'000ull) return;
	wlr_log(WLR_INFO,
	        "zenith:frame-timing output=%s stage=%s samples=%llu avg_ms=%.3f max_ms=%.3f blocked=%llu",
	        output, stage, (unsigned long long) sample.count,
	        double(sample.total) / double(sample.count) / 1e6,
	        double(sample.maximum) / 1e6, (unsigned long long) sample.blocked);
	sample = {};
	sample.since = now;
}

class FrameTimingScope {
public:
	FrameTimingScope(const char* stage, const char* output)
		: stage_(stage), output_(output), start_(frame_trace_now()) {}
	~FrameTimingScope() {
		if (start_ != 0) trace_frame_sample(stage_, output_, frame_trace_now() - start_);
	}
private:
	const char* stage_;
	const char* output_;
	uint64_t start_;
};

} // namespace zenith::render
