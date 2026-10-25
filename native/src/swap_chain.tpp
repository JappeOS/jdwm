#pragma once

#include <memory>
#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>

extern "C" {
#include <wlr/render/allocator.h>
#define static
#include <wlr/render/gles2.h>
#undef static
}

static inline uint64_t zenith_swap_chain_monotonic_now_ns() {
	timespec now{};
	clock_gettime(CLOCK_MONOTONIC, &now);
	return static_cast<uint64_t>(now.tv_sec) * 1'000'000'000ull + static_cast<uint64_t>(now.tv_nsec);
}

template<class T>
Slot<T>::Slot(std::shared_ptr<T> buffer) : buffer{buffer} {
}

template<class T>
Slot<T>::~Slot() {
	int fence_fd = ready_fence_fd.exchange(-1, std::memory_order_acq_rel);
	ready_fence_set_ns.store(0, std::memory_order_release);
	if (fence_fd != -1) {
		close(fence_fd);
	}
}

template<class T>
void Slot<T>::acquire_presentation() {
	presentation_refs.fetch_add(1, std::memory_order_relaxed);
}

template<class T>
void Slot<T>::release_presentation() {
	uint32_t refs = presentation_refs.load(std::memory_order_relaxed);
	while (refs > 0) {
		if (presentation_refs.compare_exchange_weak(refs, refs - 1, std::memory_order_relaxed)) {
			return;
		}
	}
}

template<class T>
bool Slot<T>::is_presented() const {
	return presentation_refs.load(std::memory_order_relaxed) > 0;
}

template<class T>
void Slot<T>::set_ready_fence_fd(int fd) {
	std::scoped_lock lock(fence_mutex);
	ready_fence_set_ns.store(fd != -1 ? zenith_swap_chain_monotonic_now_ns() : 0, std::memory_order_release);
	int previous = ready_fence_fd.exchange(fd, std::memory_order_acq_rel);
	if (previous != -1 && previous != fd) {
		close(previous);
	}
}

template<class T>
bool Slot<T>::is_ready_nonblocking() {
	// Output selection and raster admission can check the same completed
	// slot concurrently. Serialize poll/close with duplication and replacement
	// so descriptor reuse cannot turn an unrelated fd into a completion signal.
	std::scoped_lock lock(fence_mutex);
	int fence_fd = ready_fence_fd.load(std::memory_order_acquire);
	if (fence_fd == -1) {
		return true;
	}

	pollfd pfd{};
	pfd.fd = fence_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int ret = poll(&pfd, 1, 0);
	if (ret > 0 && (pfd.revents & POLLIN)) {
		int consumed = ready_fence_fd.exchange(-1, std::memory_order_acq_rel);
		ready_fence_set_ns.store(0, std::memory_order_release);
		if (consumed != -1) {
			close(consumed);
		}
		return true;
	}
	// An error or timeout is not GPU completion. Keep the fence pending and
	// let the reader select another completed frame instead of sampling a
	// buffer that the GPU may still be writing.

	return false;
}

template<class T>
int Slot<T>::duplicate_ready_fence() {
	std::scoped_lock lock(fence_mutex);
	const int fd = ready_fence_fd.load(std::memory_order_acquire);
	if (fd == -1) {
		errno = 0;
		return -1;
	}
	return fcntl(fd, F_DUPFD_CLOEXEC, 0);
}

template<class T>
SwapChain<T>::SwapChain(std::vector<std::shared_ptr<T>> buffers) {
	assert(buffers.size() >= 4);
	slots.reserve(buffers.size());
	for (const auto& buffer : buffers) {
		slots.emplace_back(std::make_shared<Slot<T>>(buffer));
	}
	write_buffer = slots[slots.size() - 1];
}

template<class T>
std::shared_ptr<Slot<T>> SwapChain<T>::choose_write_buffer_locked() const {
	if (write_in_progress && write_buffer != nullptr &&
	    write_buffer != latest_buffer &&
	    !write_buffer->is_presented()) {
		return write_buffer;
	}

	std::shared_ptr<Slot<T>> oldest;
	std::shared_ptr<Slot<T>> newest_ready;
	for (const auto& candidate : slots) {
		if (candidate->frame_serial.load() != 0 && candidate->is_ready_nonblocking() &&
		    (newest_ready == nullptr || candidate->frame_serial.load() > newest_ready->frame_serial.load())) {
			newest_ready = candidate;
		}
	}
	for (const auto& candidate : slots) {
		// Preserve a completed frame for the reader even if all newer frames
		// are GPU-pending. Otherwise the writer can continually steal the only
		// frame the output could advance to, starving presentation under load.
		if (candidate == nullptr || candidate == latest_buffer || candidate == newest_ready) {
			continue;
		}
		if (!candidate->is_presented() && candidate->is_ready_nonblocking() &&
		    (oldest == nullptr || candidate->frame_serial.load() < oldest->frame_serial.load())) {
			oldest = candidate;
		}
	}
	// Never overwrite a frame being read/submitted by an output. In
	// particular, a slow output on another GPU may still own every slot.
	return oldest;
}

template<class T>
T* SwapChain<T>::start_write() {
	std::unique_lock lock(mutex);
	for (;;) {
		write_buffer = choose_write_buffer_locked();
		write_in_progress = write_buffer != nullptr;
		if (write_buffer != nullptr) return write_buffer->buffer.get();

		// Flutter may already have pipelined work when vsync admission stops.
		// Bound that work here too: never replace an unfinished slot's fence.
		std::shared_ptr<Slot<T>> pending;
		for (const auto& candidate : slots) {
			if (candidate->is_presented() ||
			    candidate->ready_fence_fd.load() == -1) continue;
			if (pending == nullptr || candidate->frame_serial.load() < pending->frame_serial.load()) {
				pending = candidate;
			}
		}
		if (pending == nullptr) return nullptr; // All remaining buffers are leased.
		// A reader can consume/close the slot's fence while we wait. Own a
		// duplicate, and release the mutex so output selection never waits on us.
		int fd = pending->duplicate_ready_fence();
		if (fd == -1) {
			if (errno == 0) continue; // A reader already consumed this fence.
			return nullptr;
		}
		lock.unlock();
		pollfd pfd{fd, POLLIN, 0};
		int result;
		do { result = poll(&pfd, 1, -1); } while (result < 0 && errno == EINTR);
		close(fd);
		if (result < 0 || !(pfd.revents & POLLIN)) return nullptr;
		lock.lock();
		// Re-evaluate both leases and the newest completed frame after waking.
	}
}

template<class T>
bool SwapChain<T>::latest_frame_ready() {
	std::scoped_lock lock(mutex);
	return latest_buffer == nullptr || latest_buffer->is_ready_nonblocking();
}

template<class T>
array_view<FlutterRect> SwapChain<T>::get_damage_regions() {
	std::scoped_lock lock(mutex);
	if (write_buffer == nullptr) {
		return array_view<FlutterRect>(nullptr, 0);
	}
	auto& damage = write_buffer->damage_regions;
	return array_view<FlutterRect>(damage.data(), damage.size());
}

template<class T>
void SwapChain<T>::end_write(array_view<FlutterRect> damage, int ready_fence_fd) {
	std::scoped_lock lock(mutex);
	if (write_buffer == nullptr) {
		if (ready_fence_fd != -1) {
			close(ready_fence_fd);
		}
		return;
	}
	write_buffer->damage_regions = std::vector(damage.begin(), damage.end());
	write_buffer->set_ready_fence_fd(ready_fence_fd);
	const uint64_t serial = next_frame_serial.fetch_add(1, std::memory_order_relaxed);
	write_buffer->frame_serial.store(serial, std::memory_order_release);
	latest_buffer = write_buffer;
	write_in_progress = false;
}

template<class T>
std::shared_ptr<Slot<T>> SwapChain<T>::start_read_slot(bool* latest_pending) {
	std::scoped_lock lock(mutex);
	if (latest_pending != nullptr) *latest_pending = false;
	if (latest_buffer == nullptr) {
		return nullptr;
	}
	auto slot = latest_buffer;
	if (!latest_buffer->is_ready_nonblocking()) {
		if (latest_pending != nullptr) *latest_pending = true;
		// Don't chase an unfinished newest frame while completed frames are
		// waiting in the queue. This is common when GPU work runs behind CPU
		// submission. The current write target is not a completed candidate.
		std::shared_ptr<Slot<T>> ready;
		for (const auto& candidate : slots) {
			if ((write_in_progress && candidate == write_buffer) ||
			    candidate->frame_serial.load() == 0) continue;
			if ((ready == nullptr || candidate->frame_serial.load() > ready->frame_serial.load()) &&
			    candidate->is_ready_nonblocking()) ready = candidate;
		}
		if (ready != nullptr) slot = ready;
	}
	// Pin under the same mutex used by the writer's selection. Pinning only
	// after output commit leaves a race where this slot can be overwritten
	// while the output is checking its fence or submitting its buffer.
	slot->acquire_presentation();
	return std::shared_ptr<Slot<T>>(slot.get(), [slot](Slot<T>*) {
		slot->release_presentation();
	});
}

template<class T>
T* SwapChain<T>::start_read() {
	std::shared_ptr<Slot<T>> slot = start_read_slot();
	return slot != nullptr ? slot->buffer.get() : nullptr;
}

template<class T>
SwapChain<T>::~SwapChain() {
	std::scoped_lock lock(mutex);
	slots.clear();
	write_buffer = nullptr;
	latest_buffer = nullptr;
}
