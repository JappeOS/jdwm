#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>
#include "util/array_view.hpp"
#include "third_party/embedder.h"

extern "C" {
#include <wlr/types/wlr_buffer.h>
}

template<class T>
struct Slot {
	explicit Slot(std::shared_ptr<T> buffer);

	std::shared_ptr<T> buffer;
	std::vector<FlutterRect> damage_regions = {};
	std::atomic<uint32_t> presentation_refs{0};

	void acquire_presentation();
	void release_presentation();
	[[nodiscard]] bool is_presented() const;
};

template<class T>
struct SwapChain {
	explicit SwapChain(std::vector<std::shared_ptr<T>> buffers);

	std::mutex mutex = {};

	std::vector<std::shared_ptr<Slot<T>>> slots = {};
	std::shared_ptr<Slot<T>> write_buffer = {};
	std::shared_ptr<Slot<T>> latest_buffer = {};

	[[nodiscard]] T* start_write();

	[[nodiscard]] array_view<FlutterRect> get_damage_regions();

	void end_write(array_view<FlutterRect> damage);

	[[nodiscard]] T* start_read();
	[[nodiscard]] std::shared_ptr<Slot<T>> start_read_slot();

	virtual ~SwapChain();

private:
	[[nodiscard]] std::shared_ptr<Slot<T>> choose_write_buffer_locked() const;
};

#include "swap_chain.tpp"
