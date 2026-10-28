#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include "third_party/embedder.h"
#include "util/array_view.hpp"

namespace zenith::render {

enum class DamageReason { None, Missing, Invalid, FirstUse, HistoryLost, Generation, Forced, Readback };

// Coordinates are composition pixels, with a top-left origin. Empty and
// unknown are deliberately different: unknown always requires full repair.
struct Damage {
	FlutterRect rect{};
	DamageReason reason = DamageReason::None;

	bool empty() const { return rect.right <= rect.left || rect.bottom <= rect.top; }

	double area() const { return empty() ? 0 : (rect.right - rect.left) * (rect.bottom - rect.top); }

	void unite(const Damage& other) {
		if (other.reason != DamageReason::None)
			reason = other.reason;

		if (other.empty())
			return;

		if (empty()) {
			rect = other.rect;
			return;
		}

		rect = {std::min(rect.left, other.rect.left), std::min(rect.top, other.rect.top),
		        std::max(rect.right, other.rect.right), std::max(rect.bottom, other.rect.bottom)};
	}

	bool intersects(const FlutterRect& box) const {
		return !empty() && rect.left < box.right && box.left < rect.right &&
		       rect.top < box.bottom && box.top < rect.bottom;
	}
};

inline bool force_full_repaint() {
	static const bool enabled = [] {
		const char* value = std::getenv("ZENITH_FORCE_FULL_REPAINT");
		return value && value[0] && value[0] != '0';
	}();
	return enabled;
}

// Externally synchronized by the owning swapchain. Only frame deltas enter
// the journal: repair damage must not feed back into subsequent frame deltas.
class DamageHistory {
public:
	static constexpr size_t capacity = 256;

	DamageHistory(int width, int height) : width_(width), height_(height), generation_(++next_generation_) {}

	uint64_t generation() const { return generation_; }
	uint64_t latest() const { return latest_; }
	int width() const { return width_; }
	int height() const { return height_; }

	Damage full(DamageReason reason) const {
		return {{0, 0, double(width_), double(height_)}, reason};
	}

	Damage normalize(array_view<FlutterRect> rects) const {
		if (rects.size() == 0)
			return full(DamageReason::Missing);

		Damage result;
		for (const auto& r : rects) {
			if (!std::isfinite(r.left) || !std::isfinite(r.top) || !std::isfinite(r.right) ||
			    !std::isfinite(r.bottom) || r.right < r.left || r.bottom < r.top)
				return full(DamageReason::Invalid);

			if (r.right == r.left || r.bottom == r.top)
				continue;

			result.unite({{std::clamp(std::floor(r.left), 0.0, double(width_)),
			               std::clamp(std::floor(r.top), 0.0, double(height_)),
			               std::clamp(std::ceil(r.right), 0.0, double(width_)),
			               std::clamp(std::ceil(r.bottom), 0.0, double(height_))}, DamageReason::None});
		}

		return result;
	}

	void publish(uint64_t serial, Damage damage) {
		entries_.push_back({serial, damage});
		latest_ = serial;
		if (entries_.size() > capacity)
			entries_.pop_front();
	}

	Damage since(uint64_t generation, uint64_t after, uint64_t through) const {
		if (generation != generation_)
			return full(DamageReason::Generation);

		if (after == 0)
			return full(DamageReason::FirstUse);

		if (through < after || through > latest_)
			return full(DamageReason::Invalid);

		if (through == after)
			return {};

		if (entries_.empty() || after < entries_.front().serial - 1)
			return full(DamageReason::HistoryLost);

		Damage result;
		for (const auto& entry : entries_) {
			if (entry.serial > after && entry.serial <= through)
				result.unite(entry.damage);
		}
		return result;
	}
private:
	struct Entry { uint64_t serial; Damage damage; };
	int width_, height_;
	uint64_t generation_, latest_ = 0;
	inline static std::atomic<uint64_t> next_generation_{0};
	std::deque<Entry> entries_;
};
}
