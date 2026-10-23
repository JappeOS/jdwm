#include "output/frame_selection.hpp"

#include <cassert>
#include <sys/eventfd.h>

struct TestOutput {
	SwapChain<int> chain{{std::make_shared<int>(0), std::make_shared<int>(0),
	                      std::make_shared<int>(0), std::make_shared<int>(0)}};
	wl_event_source* timer = nullptr;
	int selected = 0;
	int callbacks = 0;

	void repaint() {
		auto frame = zenith::render::select_output_frame(chain, timer);
		assert(frame.slot);
		selected = *frame.slot->buffer;
	}
};

int main() {
	auto* loop = wl_event_loop_create();
	assert(loop);
	TestOutput output;
	output.timer = wl_event_loop_add_timer(loop, [](void* data) {
		auto* output = static_cast<TestOutput*>(data);
		++output->callbacks;
		output->repaint();
		return 0;
	}, &output);
	assert(output.timer);
	FlutterRect rect{0, 0, 100, 100};
	auto damage = array_view<FlutterRect>(&rect, 1);
	*output.chain.start_write() = 1;
	output.chain.end_write(damage);
	output.repaint();
	assert(output.selected == 1);

	// Wallpaper is the final submitted frame. It is not GPU-ready when the
	// repaint runs, so the old desktop is selected and has no new damage.
	int fence = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	assert(fence >= 0);
	*output.chain.start_write() = 2;
	output.chain.end_write(damage, dup(fence));
	output.repaint();
	assert(output.selected == 1);

	// The retry must stay alive even across more than one pending check.
	assert(wl_event_loop_dispatch(loop, 50) == 0);
	assert(output.callbacks == 1 && output.selected == 1);
	assert(eventfd_write(fence, 1) == 0);
	// No mouse event, new client commit, or Flutter frame is sent here.
	assert(wl_event_loop_dispatch(loop, 50) == 0);
	assert(output.callbacks == 2 && output.selected == 2);
	assert(wl_event_loop_dispatch(loop, 0) == 0);
	assert(output.callbacks == 2); // No permanent repaint loop after readiness.
	close(fence);
	wl_event_source_remove(output.timer);
	wl_event_loop_destroy(loop);
}
