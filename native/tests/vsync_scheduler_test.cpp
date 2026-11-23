#include "flutter_engine/vsync_scheduler.hpp"

#include <cassert>

int main() {
	const auto ready = [] { return true; };
	const auto full = [] { return false; };
	constexpr uint64_t interval = 1'000'000'000ull / 144;
	constexpr uint64_t start = 1'000'000'000ull;
	VsyncScheduler scheduler;
	assert(!scheduler.poll(start, interval, ready).baton);
	scheduler.request(1);
	auto first = scheduler.poll(start, interval, ready);
	assert(first.baton == 1 && first.target_ns == start + interval);
	scheduler.request(2);
	// Thousands of no-damage output callbacks cannot consume a second baton
	// in the same refresh interval, or overwrite the original deadline.
	for (uint64_t i = 0; i < 6000; ++i) {
		auto waiting = scheduler.poll(start + i * 1000, interval, ready);
		assert(!waiting.baton);
		assert(waiting.delay_ns == interval - i * 1000);
	}
	auto second = scheduler.poll(first.target_ns, interval, ready);
	assert(second.baton == 2);
	assert(second.target_ns == start + 2 * interval);
	// A full GPU queue before the deadline can wait for refresh instead of
	// waking the Wayland loop every millisecond to retry admission.
	VsyncScheduler paced;
	paced.request(1);
	assert(paced.poll(start, interval, ready).baton);
	paced.request(2);
	int capacity_checks = 0;
	auto early_retry = paced.poll(start + 100'000, interval, [&] { ++capacity_checks; return false; });
	assert(capacity_checks == 0);
	assert(!early_retry.baton && !early_retry.gpu_blocked);
	assert(early_retry.delay_ns == interval - 100'000);
	assert(paced.poll(start + interval, interval, full).gpu_blocked);
	assert(paced.poll(start + interval + 1'000'000, interval, ready).baton == 2);

	assert(!scheduler.poll(second.target_ns, interval, ready).baton);
	assert(scheduler.poll(second.target_ns, interval, ready).delay_ns == 0);

	// A stalled or idle compositor resumes once, with no catch-up burst.
	scheduler.request(3);
	const uint64_t resumed = start + 100 * interval + 1234;
	auto late = scheduler.poll(resumed, interval, ready);
	assert(late.baton == 3 && late.target_ns > resumed);
	scheduler.request(4);
	assert(!scheduler.poll(resumed, interval, ready).baton);
	assert(scheduler.poll(late.target_ns, interval, ready).baton == 4);

	// Event-loop jitter must not shift the deadline by a millisecond every
	// frame or cause a refresh-boundary callback to miss an entire refresh.
	scheduler.request(5);
	auto early = scheduler.poll(late.target_ns + interval - 100'000, interval, ready);
	assert(early.baton == 5);
	assert(early.target_ns == late.target_ns + 2 * interval - 100'000);

	// Match the new driver output if its refresh rate changes.
	scheduler.request(6);
	constexpr uint64_t slower = 1'000'000'000ull / 60;
	auto changed = scheduler.poll(resumed + 3 * interval, slower, ready);
	assert(changed.baton == 6);
	assert(changed.target_ns == resumed + 3 * interval + slower);

	// Simulate Flutter requesting the next tick immediately after every reply
	// while output callbacks arrive at 10 kHz with no page-flip to pace them.
	VsyncScheduler busy;
	busy.request(1);
	int delivered = 0;
	for (uint64_t now = start; now < start + 1'000'000'000ull; now += 100'000) {
		auto dispatch = busy.poll(now, interval, ready);
		if (dispatch.baton) {
			assert(dispatch.target_ns > now);
			++delivered;
			busy.request(delivered + 1);
		}
	}
	assert(delivered >= 144 && delivered <= 145);

	// A timer can reach many refresh deadlines while the GPU is behind.
	// Keep the same request pending and resume once, without catch-up ticks.
	VsyncScheduler overloaded;
	assert(overloaded.poll(start, interval, full).delay_ns == 0);
	overloaded.request(42);
	for (uint64_t i = 0; i < 1000; ++i) {
		auto stalled = overloaded.poll(start + i * interval, interval, full);
		assert(!stalled.baton && stalled.gpu_blocked && stalled.delay_ns > 0);
	}
	const auto recovered_at = start + 1000 * interval;
	auto recovered = overloaded.poll(recovered_at, interval, ready);
	assert(recovered.baton == 42 && !recovered.gpu_blocked);
	assert(recovered.target_ns == recovered_at + interval);
	assert(!overloaded.poll(recovered_at, interval, ready).baton);
	overloaded.request(43);
	assert(!overloaded.poll(recovered_at, interval, ready).baton);
	assert(overloaded.poll(recovered_at + interval, interval, ready).baton == 43);
}
