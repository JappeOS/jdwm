#pragma once

#include <cstdint>

extern "C" {
#include <wlr/types/wlr_compositor.h>
}

// A commit also carries input/opaque regions, frame callbacks and viewport
// state. Those do not change the underlying pixel image. A new attachment must
// still refresh it even with no damage, and damage can update a retained buffer.
inline bool surface_commit_updates_texture(uint32_t committed) {
	return (committed & (WLR_SURFACE_STATE_BUFFER |
		WLR_SURFACE_STATE_SURFACE_DAMAGE | WLR_SURFACE_STATE_BUFFER_DAMAGE)) != 0;
}
