#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include <mutex>
#include "util/array_view.hpp"
#include "third_party/embedder.h"
#include "output/damage_history.hpp"

extern "C" {
#include <wlr/types/wlr_buffer.h>
}

template<class T>
struct Slot {
	explicit Slot(std::shared_ptr<T> buffer);
	~Slot();

	std::shared_ptr<T> buffer;
	uint64_t generation = 0;
	std::atomic<uint32_t> presentation_refs{0};
	std::atomic<int> ready_fence_fd{-1};
	std::atomic<uint64_t> ready_fence_set_ns{0};
	std::atomic<uint64_t> frame_serial{0};

	void acquire_presentation();
	void release_presentation();
	[[nodiscard]] bool is_presented() const;
	void set_ready_fence_fd(int fd);
	[[nodiscard]] bool is_ready_nonblocking();
	// Caller owns the returned fd. -1 with errno == 0 means already ready.
	[[nodiscard]] int duplicate_ready_fence();

private:
	std::mutex fence_mutex;
};

template<class T>
struct SwapChain {
	explicit SwapChain(std::vector<std::shared_ptr<T>> buffers, int width = 0, int height = 0);
	zenith::render::DamageHistory damage_history;

	std::mutex mutex = {};

	std::vector<std::shared_ptr<Slot<T>>> slots = {};
	std::shared_ptr<Slot<T>> write_buffer = {};
	std::shared_ptr<Slot<T>> latest_buffer = {};
	bool write_in_progress = false;
	std::atomic<uint64_t> next_frame_serial{1};

	// Raster/producer thread only: may wait for GPU completion, without
	// holding the chain mutex. Readers on the Wayland thread never wait.
	[[nodiscard]] T* start_write();

	[[nodiscard]] zenith::render::Damage repair_damage();
	[[nodiscard]] zenith::render::Damage damage_since(uint64_t generation, uint64_t after, uint64_t through);

	void end_write(array_view<FlutterRect> damage, int ready_fence_fd = -1);

	[[nodiscard]] T* start_read();
	[[nodiscard]] std::shared_ptr<Slot<T>> start_read_slot(bool* latest_pending = nullptr);
	// Nonblocking GPU admission check for the Wayland-thread vsync scheduler.
	[[nodiscard]] bool latest_frame_ready();

	virtual ~SwapChain();

private:
	[[nodiscard]] std::shared_ptr<Slot<T>> choose_write_buffer_locked() const;
};

#include "swap_chain.tpp"
