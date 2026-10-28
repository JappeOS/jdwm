#include "output/flutter_damage_policy.hpp"

#include <array>
#include <cassert>

using namespace zenith::render;

// Reference compositor with an opaque striped background and a translucent
// backdrop-filtered widget. Pixels outside a partial repair retain the previous
// final composition, not the pre-filter background required by a readback.
using Image = std::array<double, 32 * 32>;

static Image render(Image image, const Damage& clip, bool hover = false, bool tooltip = false) {
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
	for (int y = 12; y < 20; ++y)
		for (int x = 12; x < 20; ++x)
			if (hover && inside(x, y)) image[y * 32 + x] *= 0.7;
	for (int y = 4; y < 10; ++y)
		for (int x = 6; x < 20; ++x)
			if (tooltip && inside(x, y)) image[y * 32 + x] = 0.2;
	return image;
}

int main() {
	FlutterOpenGLRendererConfig config{};
	config.populate_existing_damage = [](void*, intptr_t, FlutterDamage*) {};
	configure_flutter_damage(config);
	assert(config.populate_existing_damage == nullptr);

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

	auto safe_clip = flutter_composition_damage(history);
	assert(safe_clip.reason == (force_full_repaint() ? DamageReason::Forced : DamageReason::Readback));
	assert(safe_clip.area() == 32 * 32);
	safe_clip.unite(current_frame);
	assert(render(reference, safe_clip) == reference);

	// The old safeguard left a current buffer on the incremental path. A small
	// hover update can still read pixels from its previous final composition.
	Damage hover_hint{{12, 12, 20, 20}, DamageReason::None};
	assert(render(reference, hover_hint, true) != render({}, full, true));

	// Alternate hover and tooltip appearance/removal across three reused buffers.
	// Both rendering and presentation must replace the complete scene, including
	// when the engine would supply an empty damage hint. No widget style changes.
	std::array<Image, 3> buffers{};
	Image output{};
	DamageHistory presented(32, 32);
	uint64_t accepted = 0;
	for (uint64_t frame = 1; frame <= 24; ++frame) {
		const bool hover = frame % 4 < 2;
		const bool tooltip = frame % 4 == 1 || frame % 4 == 2;
		auto expected = render({}, full, hover, tooltip);
		auto damage = flutter_composition_damage(presented);
		auto& buffer = buffers[frame % buffers.size()];
		buffer = render(buffer, damage, hover, tooltip);
		assert(buffer == expected);
		presented.publish(frame, damage);
		if (frame % 5 == 0) continue; // Output skips a frame.
		auto output_damage = presented.since(presented.generation(), accepted, frame);
		for (int y = int(output_damage.rect.top); y < output_damage.rect.bottom; ++y)
			for (int x = int(output_damage.rect.left); x < output_damage.rect.right; ++x)
				output[y * 32 + x] = buffer[y * 32 + x];
		accepted = frame;
		assert(output == expected);
	}

	// DamageHistory itself still supports precise deltas. The compatibility
	// policy applies only at the Flutter producer boundary, not to its utilities.
	assert(history.since(history.generation(), 1, 2).area() == 4);
	history.publish(3, current_frame);
	assert(history.since(history.generation(), 2, 3).area() == 4);
}
