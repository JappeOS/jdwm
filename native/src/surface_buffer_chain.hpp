#pragma once

#include <cassert>
#include <memory>

template<class T>
struct SurfaceBufferChain {
	std::shared_ptr<T> newest_buffer = {};

	// Each reader owns its frame until that specific frame is released.
	[[nodiscard]] std::shared_ptr<T> start_read();

	void commit_buffer(std::shared_ptr<T> buffer);

	virtual ~SurfaceBufferChain();
};

#include "surface_buffer_chain.tpp"
