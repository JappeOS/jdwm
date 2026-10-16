#pragma once

#include <thread>

// Avoid circular dependency on `server.hpp`.
bool is_main_thread();

template<class T>
std::shared_ptr<T> SurfaceBufferChain<T>::start_read() {
	assert(is_main_thread());
	return newest_buffer;
}

template<class T>
void SurfaceBufferChain<T>::commit_buffer(std::shared_ptr<T> buffer) {
	assert(is_main_thread());
	newest_buffer = std::move(buffer);
}

template<class T>
SurfaceBufferChain<T>::~SurfaceBufferChain() {
	assert(is_main_thread());
	newest_buffer = nullptr;
}
