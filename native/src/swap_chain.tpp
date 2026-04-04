#pragma once

#include <memory>
#include <cassert>

extern "C" {
#include <wlr/render/allocator.h>
#define static
#include <wlr/render/gles2.h>
#undef static
}

template<class T>
Slot<T>::Slot(std::shared_ptr<T> buffer) : buffer{buffer} {
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
SwapChain<T>::SwapChain(std::vector<std::shared_ptr<T>> buffers) {
	assert(buffers.size() >= 4);
	slots.reserve(buffers.size());
	for (const auto& buffer : buffers) {
		slots.emplace_back(std::make_shared<Slot<T>>(buffer));
	}
	latest_buffer = slots[slots.size() - 2];
	write_buffer = slots[slots.size() - 1];
}

template<class T>
std::shared_ptr<Slot<T>> SwapChain<T>::choose_write_buffer_locked() const {
	if (write_buffer != nullptr &&
	    write_buffer != latest_buffer &&
	    !write_buffer->is_presented()) {
		return write_buffer;
	}

	for (const auto& candidate : slots) {
		if (candidate == nullptr || candidate == latest_buffer) {
			continue;
		}
		if (!candidate->is_presented()) {
			return candidate;
		}
	}
	return write_buffer != nullptr ? write_buffer : latest_buffer;
}

template<class T>
T* SwapChain<T>::start_write() {
	std::scoped_lock lock(mutex);
	write_buffer = choose_write_buffer_locked();
	if (write_buffer == nullptr) {
		return nullptr;
	}
	return write_buffer->buffer.get();
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
void SwapChain<T>::end_write(array_view<FlutterRect> damage) {
	std::scoped_lock lock(mutex);
	if (write_buffer == nullptr) {
		return;
	}
	write_buffer->damage_regions = std::vector(damage.begin(), damage.end());
	latest_buffer = write_buffer;
}

template<class T>
std::shared_ptr<Slot<T>> SwapChain<T>::start_read_slot() {
	std::scoped_lock lock(mutex);
	return latest_buffer;
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
