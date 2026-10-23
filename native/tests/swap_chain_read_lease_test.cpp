#include "swap_chain.hpp"

#include <cassert>
#include <memory>
#include <vector>

int main() {
	std::vector<std::shared_ptr<int>> buffers;
	for (int i = 0; i < 4; ++i) buffers.push_back(std::make_shared<int>(i));
	SwapChain<int> chain(buffers);
	FlutterRect damage{0, 0, 100, 100};
	auto damaged = array_view<FlutterRect>(&damage, 1);
	auto* initial = chain.start_write();
	assert(initial != nullptr);
	*initial = 100;
	chain.end_write(damaged);
	auto reading = chain.start_read_slot();
	assert(reading->is_presented());
	// Simulate a delayed output submission while the producer keeps rendering.
	// An output must see stable pixels and metadata throughout that delay.
	const auto serial = reading->frame_serial.load();
	for (int i = 0; i < 20; ++i) {
		auto* next = chain.start_write();
		assert(next != nullptr && next != reading->buffer.get());
		*next = i;
		chain.end_write(damaged);
		assert(*reading->buffer == 100);
		assert(reading->frame_serial.load() == serial);
	}
	auto slot = chain.slots.back();
	assert(slot.get() == reading.get());
	auto another_output = reading;
	reading.reset();
	assert(slot->is_presented());
	another_output.reset();
	assert(!slot->is_presented());

	// Exhaustion must not fall back to overwriting a displayed frame.
	for (auto& candidate : chain.slots) candidate->acquire_presentation();
	assert(chain.start_write() == nullptr);
	for (auto& candidate : chain.slots) candidate->release_presentation();
	assert(chain.start_write() != nullptr);
}
