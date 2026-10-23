#pragma once

#include "swap_chain.hpp"

extern "C" {
#include <wayland-server-core.h>
}

namespace zenith::render {

template<class T>
struct OutputFrameSelection {
	std::shared_ptr<Slot<T>> slot;
	bool latest_pending = false;
};

template<class T>
OutputFrameSelection<T> select_output_frame(SwapChain<T>& chain, wl_event_source* retry_timer) {
	OutputFrameSelection<T> selection;
	selection.slot = chain.start_read_slot(&selection.latest_pending);
	if (selection.latest_pending) {
		// Selecting an older ready frame must not consume the only repaint
		// request for a newer GPU-pending frame. Keep checking even if this
		// output has no damage and Flutter stops producing frames/input.
		wl_event_source_timer_update(retry_timer, 4);
	}
	return selection;
}

} // namespace zenith::render
