#include "flutter_engine/external_texture_registry.hpp"

#include <cassert>
#include <chrono>
#include <future>
#include <thread>

using namespace std::chrono_literals;

static ExternalTextureRegistry::Frame frame(uint32_t name) {
	return std::make_shared<const ExternalTextureFrame>(
		ExternalTextureFrame{0, name, 800, 600, {}});
}

int main() {
	ExternalTextureRegistry registry;
	assert(!registry.acquire(1));
	registry.publish(1, frame(10));
	auto old_frame = registry.acquire(1);
	std::weak_ptr<const ExternalTextureFrame> old_lifetime = old_frame;
	registry.publish(1, frame(11));
	assert(old_frame->name == 10);
	assert(registry.acquire(1)->name == 11);
	old_frame.reset();
	assert(old_lifetime.expired());

	// Keep displayed content for closing animations, but not cursor surfaces
	// which were never requested by Flutter.
	registry.surface_destroyed(1);
	assert(registry.acquire(1)->name == 11);
	registry.publish(2, frame(20));
	registry.surface_destroyed(2);
	assert(!registry.acquire(2));
	auto closing_frame = registry.acquire(1);
	registry.erase(1);
	assert(!registry.acquire(1));
	assert(closing_frame->name == 11);

	// Simulate three windows while the publishing/Wayland thread is idle:
	// raster acquisition must complete without it servicing any tasks.
	for (int id = 1; id <= 3; ++id) {
		registry.publish(id, frame(id));
	}
	auto raster = std::async(std::launch::async, [&] {
		for (int iteration = 0; iteration < 1000; ++iteration) {
			for (int id = 1; id <= 3; ++id) {
				assert(registry.acquire(id)->name == static_cast<uint32_t>(id));
			}
		}
	});
	assert(raster.wait_for(2s) == std::future_status::ready);
	raster.get();

	// A slow driver buffer release must not hold the registry mutex and block
	// Flutter from acquiring the replacement (or any other window's frame).
	std::promise<void> release_started;
	std::promise<void> allow_release;
	auto allowed = allow_release.get_future();
	registry.publish(4, ExternalTextureRegistry::Frame(
		new ExternalTextureFrame{}, [&](const ExternalTextureFrame* retired) {
			release_started.set_value();
			allowed.wait();
			delete retired;
		}));
	auto publisher = std::async(std::launch::async, [&] { registry.publish(4, frame(41)); });
	release_started.get_future().wait();
	auto concurrent_raster = std::async(std::launch::async, [&] {
		assert(registry.acquire(4)->name == 41);
		assert(registry.acquire(1)->name == 1);
	});
	assert(concurrent_raster.wait_for(2s) == std::future_status::ready);
	concurrent_raster.get();
	allow_release.set_value();
	publisher.get();
}
