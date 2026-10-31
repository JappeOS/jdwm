#include <cassert>
#include "util/embedder/mouse_button_mapping.hpp"
#include "util/embedder/mouse_button_tracker.hpp"

int main() {
	// Exercise the native -> Flutter -> client round trip, including hardware
	// that reports the alternative back/forward codes.
	for (const auto button : {BTN_LEFT, BTN_RIGHT, BTN_MIDDLE, BTN_SIDE, BTN_EXTRA,
		BTN_BACK, BTN_FORWARD}) {
		MouseButtonTracker tracker;
		tracker.press_button(button);
		const auto expected = button == BTN_BACK ? BTN_SIDE
			: button == BTN_FORWARD ? BTN_EXTRA : button;
		assert(flutter_mouse_button_to_linux(tracker.get_flutter_mouse_state()) ==
			static_cast<uint32_t>(expected));
		tracker.release_button(button);
		assert(tracker.get_flutter_mouse_state() == 0);
	}

	MouseButtonTracker tracker;
	tracker.press_button(BTN_SIDE);
	tracker.press_button(BTN_EXTRA);
	assert(tracker.get_flutter_mouse_state() ==
		(kFlutterPointerButtonMouseBack | kFlutterPointerButtonMouseForward));
	tracker.release_button(BTN_SIDE);
	assert(flutter_mouse_button_to_linux(tracker.get_flutter_mouse_state()) == BTN_EXTRA);
	tracker.release_button(BTN_EXTRA);
	assert(tracker.get_flutter_mouse_state() == 0);
	assert(flutter_mouse_button_to_linux(0) == 0);
	assert(flutter_mouse_button_to_linux(1LL << 20) == 0);
	assert(flutter_mouse_button_to_linux(
		kFlutterPointerButtonMouseBack | kFlutterPointerButtonMouseForward) == 0);
}
