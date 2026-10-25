#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <mutex>
#include <optional>

struct VsyncDispatch {
	std::optional<intptr_t> baton;
	uint64_t delay_ns = 0;
	uint64_t target_ns = 0;
	bool gpu_blocked = false;
};

// Flutter requests on its thread; the Wayland thread polls on output events.
// Idle/retry output events are not vblanks and must not generate extra ticks.
class VsyncScheduler {
public:
	void request(intptr_t baton) {
		std::scoped_lock lock(mutex_);
		pending_ = baton;
	}

	VsyncDispatch poll(uint64_t now_ns, uint64_t interval_ns, bool frame_ready = true) {
		assert(interval_ns > 0);
		std::scoped_lock lock(mutex_);
		if (!pending_) return {};
		// Refresh pacing alone does not bound queued GPU work. Retain the
		// request until the previous submission completes; never wait on the
		// Wayland thread or manufacture a vsync for an already consumed baton.
		if (!frame_ready) return {std::nullopt, 1'000'000, 0, true};
		if (interval_ns != interval_ns_) {
			interval_ns_ = interval_ns;
			next_tick_ns_ = 0;
		}
		// Allow small event-loop jitter around a refresh boundary, without
		// resetting phase and drifting by a full timer millisecond each frame.
		const uint64_t tolerance = std::min<uint64_t>(200'000, interval_ns / 20);
		if (next_tick_ns_ > now_ns && next_tick_ns_ - now_ns > tolerance) {
			return {std::nullopt, next_tick_ns_ - now_ns, 0};
		}
		if (next_tick_ns_ == 0) {
			next_tick_ns_ = now_ns + interval_ns;
		} else if (now_ns >= next_tick_ns_) {
			// Skip missed ticks after stalls/idle; never issue a catch-up burst.
			next_tick_ns_ += ((now_ns - next_tick_ns_) / interval_ns + 1) * interval_ns;
		} else {
			next_tick_ns_ += interval_ns;
		}
		// The pacing phase may be just ahead of now after a delayed callback.
		// Give Flutter a full refresh interval to render, not a near-zero budget.
		VsyncDispatch dispatch{pending_, 0, now_ns + interval_ns};
		pending_.reset();
		return dispatch;
	}

private:
	std::mutex mutex_;
	std::optional<intptr_t> pending_;
	uint64_t interval_ns_ = 0;
	uint64_t next_tick_ns_ = 0;
};
