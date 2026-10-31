#include "output/output_configuration.hpp"
#include "output/configuration_transaction.hpp"
#include <cassert>
#include <limits>
#include <vector>

using namespace zenith;
int main() {
	OutputSettings a{}, b{};
	a.enabled = b.enabled = true;
	a.width = b.width = 1920; a.height = b.height = 1080;
	a.refresh = b.refresh = 60000;
	b.x = 1920;
	DesktopGeometry geometry;
	assert(configuration_geometry({a,b}, 1, 16384, geometry));
	assert(geometry.width == 3840 && geometry.height == 1080);
	a.x = -1920; b.x = 0;
	assert(configuration_geometry({a,b}, 1.25, 16384, geometry));
	assert(geometry.x == -1920 && geometry.pixel_width == 4800 && geometry.pixel_height == 1350);
	a.scale = 2; b.transform = 1; a.x = -960;
	assert(configuration_geometry({a,b}, 2, 16384, geometry));
	assert(geometry.width == 2040 && geometry.height == 1920 && geometry.pixel_width == 4080);
	a.x = b.x = 0; a.scale = b.scale = 1; b.transform = 0;
	assert(configuration_geometry({a,b}, 1, 16384, geometry)); // overlapping displays
	assert(geometry.width == 1920);
	a.enabled = b.enabled = false;
	assert(configuration_geometry({a,b}, 1, 16384, geometry));
	assert(geometry.width == 0 && geometry.pixel_height == 0);
	a.enabled = true;
	a.scale = std::numeric_limits<float>::quiet_NaN();
	assert(!configuration_geometry({a}, 1, 16384, geometry));
	a.scale = 0;
	assert(!configuration_geometry({a}, 1, 16384, geometry));
	a.scale = 1; a.x = INT32_MAX;
	assert(!configuration_geometry({a}, 1, 16384, geometry));
	a.x = INT32_MIN; b.x = INT32_MAX - 1920; b.enabled = true;
	assert(!configuration_geometry({a,b}, 1, 16384, geometry));
	a.x = 0; a.transform = 8;
	assert(!configuration_geometry({a}, 1, 16384, geometry));
	a.transform = 0;
	assert(!configuration_geometry({a}, 10, 4096, geometry));

	for (bool dry : {false, true}) for (int failure = 0; failure <= 4; ++failure) {
		std::vector<int> calls;
		int hardware = 1, allocations = 0;
		auto result = run_configuration_transaction(dry,
			[&] { calls.push_back(1); return failure != 1; },
			[&] { calls.push_back(2); ++allocations; return failure != 2; },
			[&] { calls.push_back(3); hardware = 2; return failure < 3; },
			[&] { calls.push_back(4); if (failure == 4) return false; hardware = 1; return true; });
		if (failure == 1 || failure == 2) {
			assert(result == ConfigurationResult::Rejected && hardware == 1);
			assert(calls.size() == size_t(failure));
		} else if (dry) {
			assert(result == ConfigurationResult::Tested && hardware == 1 && calls.size() == 2);
		} else if (failure == 3) {
			assert(result == ConfigurationResult::Restored && hardware == 1 && calls.size() == 4);
		} else if (failure == 4) {
			assert(result == ConfigurationResult::RecoveryRequired && hardware == 2 && calls.size() == 4);
		} else {
			assert(result == ConfigurationResult::Applied && hardware == 2 && calls.size() == 3);
		}
	}
}
