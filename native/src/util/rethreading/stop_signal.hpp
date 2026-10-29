#pragma once

#include <atomic>
#include <cerrno>
#include <system_error>
#include <sys/eventfd.h>
#include <unistd.h>

// A permanent, pollable cancellation signal. Keep it alive until workers join.
class StopSignal {
public:
	StopSignal() : fd_(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
		if (fd_ < 0)
			throw std::system_error(errno, std::generic_category(), "stop eventfd");
	}

	~StopSignal() { close(fd_); }

	StopSignal(const StopSignal&) = delete;
	StopSignal& operator=(const StopSignal&) = delete;

	bool requested() const { return requested_.load(std::memory_order_acquire); }
	int fd() const { return fd_; }
	
	void request() {
		if (!requested_.exchange(true, std::memory_order_acq_rel)) {
			while (eventfd_write(fd_, 1) < 0 && errno == EINTR) {}
		}
	}
private:
	int fd_;
	std::atomic_bool requested_{false};
};
