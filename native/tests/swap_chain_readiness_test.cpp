#include "swap_chain.hpp"

#include <cassert>
#include <sys/eventfd.h>

int main() {
	std::vector<std::shared_ptr<int>> buffers;
	for (int i = 0; i < 4; ++i) buffers.push_back(std::make_shared<int>(0));
	SwapChain<int> chain(buffers);
	assert(!chain.start_read_slot()); // Never expose unrendered startup buffers.
	FlutterRect rect{0, 0, 100, 100};
	auto damage = array_view<FlutterRect>(&rect, 1);
	auto publish = [&](int value, int fence = -1) {
		auto* target = chain.start_write();
		assert(target);
		*target = value;
		chain.end_write(damage, fence);
	};

	publish(1);
	auto displayed = chain.start_read_slot();
	publish(2);
	int pending = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	assert(pending >= 0);
	publish(3, dup(pending));
	// Flutter prepares a future render target immediately after present.
	// It must not overwrite the completed frame waiting behind frame 3.
	auto* next_target = chain.start_write();
	assert(next_target && *next_target != 2);
	bool latest_pending = false;
	auto ready = chain.start_read_slot(&latest_pending);
	assert(latest_pending);
	assert(*ready->buffer == 2); // Advance beyond the displayed frame, not back to 1.
	assert(ready->is_ready_nonblocking());
	assert(ready->is_presented());

	// A fence older than the previous 500ms escape hatch is still not ready.
	chain.latest_buffer->ready_fence_set_ns.store(zenith_swap_chain_monotonic_now_ns() - 1'000'000'000ull);
	assert(!chain.latest_buffer->is_ready_nonblocking());
	assert(*chain.start_read_slot()->buffer == 2);

	assert(eventfd_write(pending, 1) == 0);
	auto newest = chain.start_read_slot(&latest_pending);
	assert(!latest_pending);
	assert(*newest->buffer == 3);
	assert(*ready->buffer == 2); // Older leases survive readiness changes.
	close(pending);
	*next_target = 4;
	chain.end_write(damage);
	assert(*chain.start_read_slot()->buffer == 4);

	// Without any completed frame, retain the pending frame for the existing
	// output retry path, rather than claiming it is ready or exposing serial 0.
	SwapChain<int> startup(buffers);
	int startup_fence = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	assert(startup_fence >= 0);
	assert(startup.start_write());
	startup.end_write(damage, dup(startup_fence));
	auto first = startup.start_read_slot(&latest_pending);
	assert(latest_pending && first);
	assert(!first->is_ready_nonblocking());
	assert(eventfd_write(startup_fence, 1) == 0);
	assert(startup.start_read_slot()->is_ready_nonblocking());
	close(startup_fence);

	// A recycled write target still has its old completed serial/fence until
	// end_write. It must not be mistaken for a usable completed frame.
	SwapChain<int> recycling(buffers);
	int delayed = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	assert(delayed >= 0);
	for (int value = 1; value <= 4; ++value) {
		assert(recycling.start_write());
		recycling.end_write(damage, value == 1 ? -1 : dup(delayed));
	}
	assert(recycling.start_write()); // Recycles serial 1, the only completed slot.
	auto still_pending = recycling.start_read_slot(&latest_pending);
	assert(latest_pending);
	assert(still_pending->frame_serial.load() == 4);
	assert(!still_pending->is_ready_nonblocking());
	close(delayed);
}
