#include "output/damage_history.hpp"
#include <cassert>
#include <limits>
#include <vector>

using namespace zenith::render;

int main() {
	DamageHistory history(80, 60);
	auto normalize = [&](FlutterRect r) { return history.normalize({&r, 1}); };
	auto rounded = normalize({1.2, 2.8, 5.1, 7.9});
	assert(rounded.rect.left == 1 && rounded.rect.top == 2 && rounded.rect.right == 6 && rounded.rect.bottom == 8);
	assert(normalize({0, 0, 0, 0}).empty());
	assert(normalize({-10, -20, 90, 80}).area() == 4800);
	assert(history.normalize({nullptr, 0}).reason == DamageReason::Missing);
	assert(normalize({0, 0, std::numeric_limits<double>::infinity(), 1}).reason == DamageReason::Invalid);
	assert(normalize({3, 2, 1, 4}).reason == DamageReason::Invalid);
	assert(history.since(history.generation(), 0, 0).reason == DamageReason::FirstUse);
	assert(history.since(history.generation() + 1, 1, 1).reason == DamageReason::Generation);

	// Four reused buffers and two asynchronous outputs. Incremental repair must
	// exactly match a full-frame reference after every render and accepted commit.
	std::vector<int> reference(80 * 60, 0);
	std::vector<std::vector<int>> buffers(4, reference), outputs(2, reference);
	uint64_t buffer_serials[4]{}, output_serials[2]{};
	auto copy_region = [](std::vector<int>& dest, const std::vector<int>& source, Damage damage) {
		for (int y = int(damage.rect.top); y < damage.rect.bottom; ++y)
			for (int x = int(damage.rect.left); x < damage.rect.right; ++x) dest[y * 80 + x] = source[y * 80 + x];
	};
	for (uint64_t serial = 1; serial <= 800; ++serial) {
		// Small asymmetric changes near both edges catch missing history and Y inversions.
		const int x = (serial * 7) % 78, y = serial % 2 ? 1 : 57;
		auto delta = serial % 11 == 0 ? Damage{} : normalize({double(x), double(y), double(x + 2), double(y + 2)});
		if (!delta.empty()) {
			for (int yy = y; yy < y + 2; ++yy)
				for (int xx = x; xx < x + 2; ++xx) reference[yy * 80 + xx] = int(serial);
		}
		// Deliberately non-round-robin slot reuse.
		const int slot = (serial / 3 + serial) % 4;
		auto repair = history.since(history.generation(), buffer_serials[slot], history.latest());
		repair.unite(delta);
		copy_region(buffers[slot], reference, repair);
		assert(buffers[slot] == reference);
		history.publish(serial, delta);
		buffer_serials[slot] = serial;
		for (int output = 0; output < 2; ++output) {
			if (serial % (output ? 17 : 3) != 0) continue;
			auto damage = history.since(history.generation(), output_serials[output], serial);
			// A failed output commit must leave its accepted serial untouched.
			if (serial % 51 == 0) continue;
			copy_region(outputs[output], buffers[slot], damage);
			output_serials[output] = serial;
			assert(outputs[output] == reference);
		}
	}
	assert(history.since(history.generation(), 1, 800).reason == DamageReason::HistoryLost);
	assert(history.since(history.generation(), 800, 799).reason == DamageReason::Invalid);
	assert(history.since(history.generation(), 800, 800).empty());
	assert(history.since(history.generation(), 700, 750).reason == DamageReason::None);
	DamageHistory replaced(80, 60);
	assert(replaced.since(history.generation(), 800, 800).reason == DamageReason::Generation);

	// Cropping is evaluated in composition-buffer coordinates. wlroots owns
	// scaling/rotation after this intersection; do not apply either twice here.
	DamageHistory desktop(200, 120);
	FlutterRect top_left{2.1, 3.2, 9.3, 10.4}, bottom_right{150, 100, 160, 110};
	desktop.publish(1, desktop.full(DamageReason::FirstUse));
	desktop.publish(2, desktop.normalize({&top_left, 1}));
	auto left_only = desktop.since(desktop.generation(), 1, 2);
	assert(left_only.intersects({0, 0, 100, 120}));
	assert(!left_only.intersects({100, 0, 200, 120}));
	assert(!left_only.intersects({0, 60, 100, 120})); // No spurious Y inversion.
	desktop.publish(3, desktop.normalize({&bottom_right, 1}));
	auto right_only = desktop.since(desktop.generation(), 2, 3);
	assert(!right_only.intersects({0, 0, 100, 120}));
	assert(right_only.intersects({100, 0, 200, 120}));
	auto skipped = desktop.since(desktop.generation(), 1, 3);
	assert(skipped.intersects({0, 0, 100, 120}) && skipped.intersects({100, 0, 200, 120}));
}
