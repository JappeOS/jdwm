#pragma once

#include <cstdint>
#include <pixman.h>

// A rectangle wholly inside the effective surface-local opaque region.
// The region's extents are not sufficient: they can include transparent holes.
inline pixman_box32_t surface_opaque_rect(pixman_region32_t* region) {
	int count = 0;
	auto* rects = pixman_region32_rectangles(region, &count);
	pixman_box32_t best{};
	int64_t area = 0;
	for (int i = 0; i < count; ++i) {
		const int64_t candidate_area = (int64_t(rects[i].x2) - rects[i].x1) *
			(int64_t(rects[i].y2) - rects[i].y1);
		if (candidate_area > area) {
			area = candidate_area;
			best = rects[i];
		}
	}
	return best;
}
