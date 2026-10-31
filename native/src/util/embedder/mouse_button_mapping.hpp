#pragma once

#include <cstdint>
#include <linux/input-event-codes.h>
#include "embedder.h"

// Convert one Flutter button bit to a Wayland/Linux button code. Zero means
// unsupported (including combined bitmasks) and must never be sent to clients.
inline uint32_t flutter_mouse_button_to_linux(int64_t button) {
	switch (button) {
		case kFlutterPointerButtonMousePrimary: return BTN_LEFT;
		case kFlutterPointerButtonMouseSecondary: return BTN_RIGHT;
		case kFlutterPointerButtonMouseMiddle: return BTN_MIDDLE;
		case kFlutterPointerButtonMouseBack: return BTN_SIDE;
		case kFlutterPointerButtonMouseForward: return BTN_EXTRA;
		default: return 0;
	}
}
