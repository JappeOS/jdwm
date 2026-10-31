#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

struct wlr_output;
struct wlr_output_mode;

namespace zenith {

// Wayland-thread values, independent of protocol request/resource lifetimes.
struct OutputSettings {
	wlr_output* output = nullptr;
	bool enabled = false;
	wlr_output_mode* mode = nullptr;
	int width = 0, height = 0, refresh = 0;
	int x = 0, y = 0;
	float scale = 1;
	int transform = 0;
	bool adaptive_sync = false;
};

using OutputConfiguration = std::vector<OutputSettings>;

struct DesktopGeometry {
	int x = 0, y = 0, width = 0, height = 0;
	int pixel_width = 0, pixel_height = 0;
};

// Use wide arithmetic before converting protocol coordinates to renderer sizes.
inline bool configuration_geometry(
	const OutputConfiguration& outputs,
	double pixel_ratio,
	int max_dimension,
	DesktopGeometry& result
) {
	result = {};
	if (!std::isfinite(pixel_ratio) || pixel_ratio <= 0 || max_dimension <= 0)
		return false;

	int64_t left = 0, top = 0, right = 0, bottom = 0;
	bool first = true;
	for (const auto& head : outputs) {
		if (!head.enabled)
			continue;

		if (!std::isfinite(head.scale) || head.scale <= 0 || head.width <= 0 ||
			head.height <= 0 || head.width > max_dimension || head.height > max_dimension || head.refresh < 0 || head.transform < 0 || head.transform > 7)
			return false;

		const double w = (head.transform & 1 ? head.height : head.width) / double(head.scale);
		const double h = (head.transform & 1 ? head.width : head.height) / double(head.scale);
		if (w < 1 || h < 1 || w > max_dimension || h > max_dimension)
			return false;

		const int64_t r = int64_t(head.x) + int64_t(w), b = int64_t(head.y) + int64_t(h);
		if (r > INT32_MAX || b > INT32_MAX)
			return false;

		left = first ? head.x : std::min(left, int64_t(head.x));
		top = first ? head.y : std::min(top, int64_t(head.y));
		right = first ? r : std::max(right, r);
		bottom = first ? b : std::max(bottom, b);
		first = false;
	}

	if (first)
		return true;

	const double pw = std::ceil((right - left) * pixel_ratio);
	const double ph = std::ceil((bottom - top) * pixel_ratio);
	if (right - left > INT32_MAX || bottom - top > INT32_MAX ||
		pw > max_dimension || ph > max_dimension)
		return false;

	result = {int(left), int(top), int(right-left), int(bottom-top), int(pw), int(ph)};
	return true;
}

} // namespace zenith
