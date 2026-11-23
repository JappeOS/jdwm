#include "swap_chain.hpp"
#include "flutter_engine/vsync_scheduler.hpp"

#include <cassert>
#include <chrono>
#include <future>
#include <deque>
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
	int fences[2];
	for (int i = 0; i < 2; ++i) {
		assert(chain.can_schedule_frame());
		fences[i] = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
		assert(fences[i] >= 0);
		publish(i + 2, dup(fences[i]));
	}
	assert(!chain.can_schedule_frame());

	// Even with a spare physical target, two unfinished submissions must stall the producer.
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
	assert(writer.wait_for(1s) == std::future_status::ready);
	auto* target = writer.get();
	assert(target && *target == 0); // The spare slot, preserving both submitted images.
	assert(chain.can_schedule_frame());
	assert(chain.start_read_slot()->frame_serial.load() == 2);
	assert(*displayed->buffer == 1);
	*target = 4;
	assert(eventfd_write(fences[1], 1) == 0);
	chain.end_write(damage);
	assert(chain.start_read_slot()->frame_serial.load() == 4);
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
	struct Submission { int fd; uint64_t completion; };
	std::deque<Submission> pending;
	uint64_t presented_serial = 0;
	int submitted = 0, advanced = 0;
	std::shared_ptr<Slot<int>> scanout;
	constexpr uint64_t interval = 1'000'000'000ull / 144;
	for (uint64_t now = 1'000'000'000; now < 2'000'000'000; now += 1'000'000) {
		if (!pending.empty() && now >= pending.front().completion) {
			assert(eventfd_write(pending.front().fd, 1) == 0);
			close(pending.front().fd);
			pending.pop_front();
		}
		auto dispatch = scheduler.poll(now, interval, [&slow] { return slow.can_schedule_frame(); });
		if (dispatch.baton) {
			assert(pending.size() < 2);
			assert(slow.start_write());
			int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
			assert(fd >= 0);
			slow.end_write(damage, dup(fd));
			auto start = pending.empty() ? now : pending.back().completion;
			pending.push_back({fd, start + 25'000'000});
			scheduler.request(++submitted + 1);
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
	assert(submitted >= 30 && submitted <= 42);
	assert(advanced >= 30 && submitted - advanced <= 2);
	for (auto submission : pending) close(submission.fd);
}
