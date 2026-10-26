#include "surfaces/opaque_region.hpp"
#include <cassert>

int main() {
	pixman_region32_t region;
	pixman_region32_init(&region);
	assert(surface_opaque_rect(&region).x2 == 0);
	// A rounded client can advertise a large interior and narrow edge strips.
	pixman_region32_union_rect(&region, &region, 50, 60, 1000, 700);
	pixman_region32_union_rect(&region, &region, 60, 50, 980, 10);
	auto result = surface_opaque_rect(&region);
	assert(result.x1 == 50 && result.y1 == 60 && result.x2 == 1050 && result.y2 == 760);
	assert(pixman_region32_contains_rectangle(&region, &result) == PIXMAN_REGION_IN);
	// A transparent hole must never be included by taking region extents.
	pixman_region32_t hole;
	pixman_region32_init_rect(&hole, 200, 100, 300, 400);
	pixman_region32_subtract(&region, &region, &hole);
	result = surface_opaque_rect(&region);
	assert(pixman_region32_contains_rectangle(&region, &result) == PIXMAN_REGION_IN);
	assert(pixman_region32_contains_rectangle(&hole, &result) == PIXMAN_REGION_OUT);
	pixman_region32_clear(&region);
	result = surface_opaque_rect(&region);
	assert(result.x1 == 0 && result.y1 == 0 && result.x2 == 0 && result.y2 == 0);
	pixman_region32_fini(&hole);
	pixman_region32_fini(&region);
}
