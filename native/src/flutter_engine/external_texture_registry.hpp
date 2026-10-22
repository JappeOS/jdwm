#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

struct wlr_buffer;

// Immutable after publication. Its deleter must release the buffer on the
// Wayland thread, even when the last reference is held by Flutter's raster thread.
struct ExternalTextureFrame {
	uint32_t target = 0;
	uint32_t name = 0;
	size_t width = 0;
	size_t height = 0;
	std::shared_ptr<wlr_buffer> buffer;
};

class ExternalTextureRegistry {
public:
	using Frame = std::shared_ptr<const ExternalTextureFrame>;

	void publish(int64_t id, Frame frame) {
		Frame retired;
		{
			std::scoped_lock lock(mutex_);
			auto& entry = frames_[id];
			retired = std::exchange(entry.frame, std::move(frame));
		}
		// Releasing a wlroots buffer may perform driver work. Never do it
		// while holding the mutex needed by the raster thread.
	}

	Frame acquire(int64_t id) {
		std::scoped_lock lock(mutex_);
		auto it = frames_.find(id);
		if (it == frames_.end()) {
			return {};
		}
		it->second.acquired = true;
		return it->second.frame;
	}

	void erase(int64_t id) { remove(id, false); }

	// Retain displayed surfaces for Flutter closing animations. Cursor surfaces
	// and other surfaces Flutter never sampled must not stay in the registry.
	void surface_destroyed(int64_t id) { remove(id, true); }

private:
	struct Entry {
		Frame frame;
		bool acquired = false;
	};
	std::mutex mutex_;
	std::unordered_map<int64_t, Entry> frames_;

	void remove(int64_t id, bool keep_acquired) {
		Frame retired;
		{
			std::scoped_lock lock(mutex_);
			auto it = frames_.find(id);
			if (it == frames_.end() || (keep_acquired && it->second.acquired)) {
				return;
			}
			retired = std::move(it->second.frame);
			frames_.erase(it);
		}
	}
};
