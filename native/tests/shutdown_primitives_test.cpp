#include "swap_chain.hpp"
#include "util/rethreading/callable_queue.hpp"

#include <cassert>
#include <chrono>
#include <future>
#include <fcntl.h>

int main() {
	using namespace std::chrono_literals;
	// A permanently unfinished GPU fence must not hold up engine shutdown.
	SwapChain<int> chain({std::make_shared<int>(0), std::make_shared<int>(0),
	                      std::make_shared<int>(0), std::make_shared<int>(0)});
	FlutterRect rect{0, 0, 32, 32};
	assert(chain.start_write());
	chain.end_write({&rect, 1});
	auto displayed = chain.start_read_slot();
	for (int i = 0; i < 3; ++i) {
		assert(chain.start_write());
		chain.end_write({&rect, 1}, eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
	}
	StopSignal stop;
	auto raster = std::async(std::launch::async, [&] { return chain.start_write(&stop); });
	assert(raster.wait_for(50ms) == std::future_status::timeout);
	stop.request();
	stop.request(); // Repeated exit requests are harmless.
	assert(raster.wait_for(1s) == std::future_status::ready);
	assert(raster.get() == nullptr);
	assert(chain.start_write(&stop) == nullptr);
	assert(chain.start_read_slot()->frame_serial == 1);

	// Work accepted before close completes before shutdown, including work
	// already moved into execute()'s private batch. Late work is rejected.
	int queue_fd;
	{
		CallableQueue queue;
		queue_fd = queue.get_fd();
		std::promise<void> entered, resume;
		auto resume_future = resume.get_future();
		int calls = 0;
		assert(queue.enqueue([&] { entered.set_value(); resume_future.wait(); ++calls; }));
		auto worker = std::async(std::launch::async, [&] {
			queue.execute();
			while (queue.execute()) {}
		});
		entered.get_future().wait();
		assert(queue.enqueue([&] { assert(calls == 1); ++calls; }));
		queue.close([&] { assert(calls == 2); ++calls; });
		queue.close([&] { assert(false); });
		assert(!queue.enqueue([&] { assert(false); }));
		resume.set_value();
		assert(worker.wait_for(1s) == std::future_status::ready);
		worker.get();
		assert(calls == 3);
	}
	assert(fcntl(queue_fd, F_GETFD) == -1 && errno == EBADF);
}
