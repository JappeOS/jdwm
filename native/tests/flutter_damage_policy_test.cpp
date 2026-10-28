#include "output/flutter_damage_policy.hpp"

#include <array>
#include <cassert>

using namespace zenith::render;

// Reference compositor with an opaque striped background and a translucent
// backdrop-filtered widget. Pixels outside a partial repair retain the previous
// final composition, not the pre-filter background required by a readback.
using Image = std::array<double, 32 * 32>;

static Image render(Image image, const Damage& clip) {
	auto inside = [&](int x, int y) {
		return x >= clip.rect.left && x < clip.rect.right &&
		       y >= clip.rect.top && y < clip.rect.bottom;
	};
	for (int y = 0; y < 32; ++y) {
		for (int x = 0; x < 32; ++x) {
			if (inside(x, y)) image[y * 32 + x] = x % 2 ? 1.0 : 0.0;
		}
	}
	const Image background = image;
	for (int y = 8; y < 24; ++y) {
		for (int x = 8; x < 24; ++x) {
			if (!inside(x, y)) continue;
			double sum = 0;
			for (int dy = -2; dy <= 2; ++dy)
				for (int dx = -2; dx <= 2; ++dx)
					sum += background[(y + dy) * 32 + x + dx];
			image[y * 32 + x] = 0.75 * (sum / 25) + 0.25; // White tint at 25% opacity.
		}
	}
	return image;
}

int main() {
	DamageHistory history(32, 32);
	const auto full = history.full(DamageReason::FirstUse);
	const auto reference = render({}, full);
	FlutterRect old_change{10, 10, 12, 12};
	history.publish(1, full);
	history.publish(2, history.normalize({&old_change, 1}));
	const auto repair = history.since(history.generation(), 1, 2);
	assert(repair.area() == 4 && repair.reason == DamageReason::None);

	// The newest frame changes somewhere outside the backdrop. In the engine's
	// current algorithm this does not expand backdrop dependencies, even though
	// the older buffer's additional repair intersects the backdrop.
	Damage current_frame{{0, 0, 2, 2}, DamageReason::None};
	auto unsafe_clip = repair;
	unsafe_clip.unite(current_frame);
	assert(render(reference, unsafe_clip) != reference);

	auto safe_clip = flutter_buffer_repair(history, repair);
	assert(safe_clip.reason == DamageReason::Readback);
	assert(safe_clip.area() == 32 * 32);
	safe_clip.unite(current_frame);
	assert(render(reference, safe_clip) == reference);

	// Do not manufacture changes when the target is already current. Flutter
	// will handle its current frame's damage and readbacks. Keep full-fallback
	// reasons intact, and keep output damage separate from the repair policy.
	assert(flutter_buffer_repair(history, {}).empty());
	assert(flutter_buffer_repair(history, full).reason == DamageReason::FirstUse);
	assert(flutter_buffer_repair(history, history.full(DamageReason::Forced)).reason == DamageReason::Forced);
	assert(history.since(history.generation(), 1, 2).area() == 4);
	history.publish(3, current_frame);
	assert(history.since(history.generation(), 2, 3).area() == 4);
}
