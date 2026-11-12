#include "window_metadata/icon_data.hpp"
#include <cassert>
#include <limits>
using namespace zenith;
int main() {
	const uint32_t pixels[] = {2, 1, 0x80ff4000, 0xff123456, 1, 1, 0x000000ff};
	auto images = parse_x11_icons(pixels, 7);
	assert(images.size() == 2);
	assert((images[0].rgba == std::vector<uint8_t>{128, 32, 0, 128, 18, 52, 86, 255}));
	assert((images[1].rgba == std::vector<uint8_t>{0, 0, 0, 0}));
	assert(parse_x11_icons(pixels, 6).empty());
	const uint32_t overflow[] = {UINT32_MAX, UINT32_MAX};
	assert(parse_x11_icons(overflow, 2).empty());
	const uint32_t zero[] = {0, 1};
	assert(parse_x11_icons(zero, 2).empty());
	std::vector<uint32_t> many;
	for (int i = 0; i < 32; ++i)
		many.insert(many.end(), {1, 1, 0xffffffff});
	assert(parse_x11_icons(many.data(), many.size()).size() == icon_variant_limit);
	assert(!valid_icon_size(1025, 1));
	assert(valid_icon_size(1024, 1024));
	uint32_t shm[] = {0x80802000, 0xdeadbeef, 0xff123456, 0xdeadbeef};
	IconImage image{1, 2, 2, {}};
	assert(copy_shm_icon(image, shm, 8, true, true));
	assert((image.rgba == std::vector<uint8_t>{128, 32, 0, 128, 18, 52, 86, 255}));
	assert(copy_shm_icon(image, shm, 8, false, false));
	assert((image.rgba == std::vector<uint8_t>{0, 32, 128, 255, 86, 52, 18, 255}));
	assert(!copy_shm_icon(image, shm, 3, true, true));
	assert(!copy_shm_icon(image, shm, SIZE_MAX, true, true));
	assert(!copy_shm_icon(image, nullptr, 8, true, true));
}
