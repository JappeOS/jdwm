#pragma once

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
};

template<class T>
struct SwapChain {
	explicit SwapChain(std::vector<std::shared_ptr<T>> buffers);

	std::mutex mutex = {};

	// Oldest-to-newest read slots. Slot 0 is the buffer returned by start_read().
	// Keeping multiple historical read buffers delays buffer reuse, which is
	// important when multiple outputs scan out asynchronously.
	std::vector<std::shared_ptr<Slot<T>>> read_buffers = {};
	std::shared_ptr<Slot<T>> write_buffer = {};
	std::shared_ptr<Slot<T>> latest_buffer = {};

	bool new_buffer_available = false;

	[[nodiscard]] T* start_write();

	[[nodiscard]] array_view<FlutterRect> get_damage_regions();

	void end_write(array_view<FlutterRect> damage);

	[[nodiscard]] T* start_read();

	virtual ~SwapChain();
};

#include "swap_chain.tpp"
