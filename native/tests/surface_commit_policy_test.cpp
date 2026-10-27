#include "surfaces/surface_commit_policy.hpp"

#include <cassert>

int main() {
	// Stationary clients can commit frame callbacks or hit-test changes on
	// every output frame. None of these may invalidate their cached image.
	constexpr uint32_t metadata[] = {
		0, WLR_SURFACE_STATE_FRAME_CALLBACK_LIST, WLR_SURFACE_STATE_INPUT_REGION,
		WLR_SURFACE_STATE_OPAQUE_REGION, WLR_SURFACE_STATE_VIEWPORT,
		WLR_SURFACE_STATE_SCALE, WLR_SURFACE_STATE_TRANSFORM, WLR_SURFACE_STATE_OFFSET,
	};
	for (uint32_t field : metadata) {
		assert(!surface_commit_updates_texture(field));
		assert(!surface_commit_updates_texture(field | WLR_SURFACE_STATE_FRAME_CALLBACK_LIST));
		// An attachment without damage can switch to a different buffer.
		assert(surface_commit_updates_texture(field | WLR_SURFACE_STATE_BUFFER));
		// Clients may damage a retained image, without attaching a new buffer.
		assert(surface_commit_updates_texture(field | WLR_SURFACE_STATE_SURFACE_DAMAGE));
		assert(surface_commit_updates_texture(field | WLR_SURFACE_STATE_BUFFER_DAMAGE));
	}
}
