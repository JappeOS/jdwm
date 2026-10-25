#include "swap_chain.hpp"
#include "flutter_engine/vsync_scheduler.hpp"

#include <cassert>
#include <chrono>
#include <future>
#include <sys/eventfd.h>

int main() {
	using namespace std::chrono_literals;
	SwapChain<int> chain({std::make_shared<int>(0), std::make_shared<int>(0),
	                      std::make_shared<int>(0), std::make_shared<int>(0)});
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
	int fences[3];
	for (int i = 0; i < 3; ++i) {
		fences[i] = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
		assert(fences[i] >= 0);
		publish(i + 2, dup(fences[i]));
	}
	assert(!chain.latest_frame_ready());

	// Fill all remaining targets with unfinished GPU work. The producer must
	// wait instead of replacing frame 2 and its fence with another submission.
	std::promise<void> started;
	auto writer = std::async(std::launch::async, [&] {
		started.set_value();
		return chain.start_write();
	});
	started.get_future().wait();
	assert(writer.wait_for(50ms) == std::future_status::timeout);
	// The main/output thread can still select while the raster thread waits.
	assert(chain.start_read_slot()->frame_serial.load() == 1);
	for (auto& slot : chain.slots) {
		if (slot->frame_serial.load() >= 2) assert(slot->ready_fence_fd.load() != -1);
	}

	assert(eventfd_write(fences[0], 1) == 0);
	assert(writer.wait_for(50ms) == std::future_status::timeout);
	// Preserve frame 2 for presentation instead of immediately overwriting
	// the only completed frame newer than the displayed one.
	assert(chain.start_read_slot()->frame_serial.load() == 2);
	assert(eventfd_write(fences[1], 1) == 0);
	assert(writer.wait_for(1s) == std::future_status::ready);
	auto* target = writer.get();
	assert(target && *target == 2);
	*target = 5;
	// Frame 3 is still available while the writer prepares its next target.
	assert(chain.start_read_slot()->frame_serial.load() == 3);
	assert(*displayed->buffer == 1);
	chain.end_write(damage);
	assert(chain.latest_frame_ready());
	assert(chain.start_read_slot()->frame_serial.load() == 5);
	for (int fd : fences) close(fd);

	// Slow secondary output: two old frames are leased and only the latest
	// GPU submission can free the completed frame reserved for presentation.
	SwapChain<int> multi({std::make_shared<int>(0), std::make_shared<int>(0),
	                      std::make_shared<int>(0), std::make_shared<int>(0)});
	auto submit = [&] {
		assert(multi.start_write());
		multi.end_write(damage);
	};
	submit();
	auto monitor_one = multi.start_read_slot();
	submit();
	auto monitor_two = multi.start_read_slot();
	submit();
	int last_fence = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	assert(last_fence >= 0);
	assert(multi.start_write());
	multi.end_write(damage, dup(last_fence));
	auto waiting = std::async(std::launch::async, [&] { return multi.start_write(); });
	assert(waiting.wait_for(50ms) == std::future_status::timeout);
	assert(multi.start_read_slot()->frame_serial.load() == 3);
	assert(eventfd_write(last_fence, 1) == 0);
	assert(waiting.wait_for(1s) == std::future_status::ready);
	assert(waiting.get());
	assert(multi.start_read_slot()->frame_serial.load() == 4);
	close(last_fence);

	// Continuous dragging on a 144 Hz output, but each GPU frame takes 25ms.
	// Rendering must advance while input continues, not only when it stops.
	SwapChain<int> slow({std::make_shared<int>(0), std::make_shared<int>(0),
	                     std::make_shared<int>(0), std::make_shared<int>(0)});
	VsyncScheduler scheduler;
	scheduler.request(1);
	assert(slow.start_write()); // Flutter prepares its FBO ahead of the tick.
	int pending_fd = -1;
	uint64_t completion = 0, presented_serial = 0;
	int submitted = 0, advanced = 0;
	std::shared_ptr<Slot<int>> scanout;
	constexpr uint64_t interval = 1'000'000'000ull / 144;
	for (uint64_t now = 1'000'000'000; now < 2'000'000'000; now += 1'000'000) {
		if (pending_fd != -1 && now >= completion) {
			assert(eventfd_write(pending_fd, 1) == 0);
			close(pending_fd);
			pending_fd = -1;
		}
		auto dispatch = scheduler.poll(now, interval, slow.latest_frame_ready());
		if (dispatch.baton) {
			assert(pending_fd == -1); // No unbounded GPU command backlog.
			pending_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
			assert(pending_fd >= 0);
			slow.end_write(damage, dup(pending_fd));
			completion = now + 25'000'000;
			++submitted;
			assert(slow.start_write());
			scheduler.request(submitted + 1);
		}
		auto frame = slow.start_read_slot();
		if (frame && frame->is_ready_nonblocking()) {
			auto serial = frame->frame_serial.load();
			assert(serial >= presented_serial);
			if (serial > presented_serial) ++advanced;
			presented_serial = serial;
			scanout = std::move(frame);
		}
	}
	assert(submitted >= 30 && submitted <= 40);
	assert(advanced >= 30 && submitted - advanced <= 1);
	if (pending_fd != -1) close(pending_fd);
}
