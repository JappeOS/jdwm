#include "swap_chain.hpp"
#include <cassert>

int main() {
	std::vector<std::shared_ptr<int>> buffers;
	for (int i = 0; i < 4; ++i) buffers.push_back(std::make_shared<int>(0));
	auto chain = std::make_shared<SwapChain<int>>(buffers, 100, 80);
	const auto generation = chain->damage_history.generation();
	FlutterRect rect{1, 2, 3, 4};
	for (int i = 1; i <= 12; ++i) {
		assert(chain->start_write());
		const auto previous = chain->write_buffer->frame_serial.load();
		const auto repair = chain->repair_damage();
		if (zenith::render::force_full_repaint()) {
			assert(repair.reason == zenith::render::DamageReason::Forced);
		} else if (!previous) {
			assert(repair.reason == zenith::render::DamageReason::FirstUse);
		} else {
			assert(repair.rect.left == previous + 1 && repair.rect.right == i + 1);
		}
		// Consecutive acquisition before presentation is the same write transaction.
		auto* writing = chain->write_buffer.get();
		assert(chain->start_write());
		assert(chain->write_buffer.get() == writing);
		rect.left = i; rect.right = i + 2;
		chain->end_write({&rect, 1});
	}
	auto cumulative = chain->damage_since(generation, 6, 10);
	assert(cumulative.rect.left == 7 && cumulative.rect.right == 12);
	auto lease = chain->start_read_slot();
	assert(lease->generation == generation);
	// A target snapshot can outlive publication of a replacement generation.
	auto raster_target = chain;
	for (auto& buffer : buffers) buffer = std::make_shared<int>(0);
	chain = std::make_shared<SwapChain<int>>(buffers, 100, 80);
	assert(chain->damage_since(generation, 12, 12).reason == zenith::render::DamageReason::Generation);
	assert(raster_target->start_write());
	raster_target->end_write({&rect, 1});
	assert(lease->frame_serial.load() == 12);

	// A failed scene commit retains the newly bound source as well as the old
	// displayed source. Subsequent producer frames must not overwrite either.
	auto displayed = chain->start_write();
	assert(displayed);
	*displayed = 1;
	chain->end_write({&rect, 1});
	auto displayed_lease = chain->start_read_slot();
	auto bound = chain->start_write();
	assert(bound);
	*bound = 2;
	chain->end_write({&rect, 1});
	auto scene_lease = chain->start_read_slot();
	for (int frame = 3; frame < 50; ++frame) {
		auto writer = chain->start_write();
		assert(writer && writer != displayed && writer != bound);
		*writer = frame;
		chain->end_write({&rect, 1});
		assert(*displayed_lease->buffer == 1 && *scene_lease->buffer == 2);
	}
}
