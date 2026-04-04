#include "presentation_timing.hpp"

extern "C" {
#include <wlr/types/wlr_output.h>
}

namespace zenith::render {

double output_refresh_hz(const wlr_output* output) {
	if (output == nullptr || output->refresh <= 0) {
		return 60.0;
	}
	return static_cast<double>(output->refresh) / 1000.0;
}

uint64_t next_presentation_time_ns(uint64_t now_ns, const wlr_output* output) {
	double refresh = output_refresh_hz(output);
	if (refresh <= 0.0) {
		refresh = 60.0;
	}
	return now_ns + static_cast<uint64_t>(1'000'000'000ull / refresh);
}

} // namespace zenith::render
