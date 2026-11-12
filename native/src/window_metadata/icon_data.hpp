#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace zenith {

constexpr size_t icon_byte_limit = 8 * 1024 * 1024;
constexpr size_t icon_variant_limit = 16;
constexpr uint32_t icon_dimension_limit = 1024;

struct IconImage {
	uint32_t width = 0, height = 0, scale = 1;
	std::vector<uint8_t> rgba;
};

struct WindowIcon {
	std::string name;
	std::vector<IconImage> images;
};

inline bool valid_icon_size(uint32_t width, uint32_t height) {
	return width && height && width <= icon_dimension_limit && height <= icon_dimension_limit;
}

// Read packed native-endian 8888 SHM pixels, preserving Wayland premultiplied alpha.
inline bool copy_shm_icon(
	IconImage &image,
	const void *pixels,
	size_t stride,
	bool argb_order,
	bool has_alpha
) {
	if (!pixels || !valid_icon_size(image.width, image.height) || !image.scale ||
		stride < size_t(image.width) * 4 || stride > SIZE_MAX / image.height)
		return false;

	image.rgba.resize(size_t(image.width) * image.height * 4);
	for (uint32_t y = 0; y < image.height; ++y) {
		for (uint32_t x = 0; x < image.width; ++x) {
			uint32_t pixel;
			memcpy(&pixel, static_cast<const uint8_t *>(pixels) + y * stride + x * 4, 4);

			auto *out = image.rgba.data() + (size_t(y) * image.width + x) * 4;
			out[0] = pixel >> (argb_order ? 16 : 0);
			out[1] = pixel >> 8;
			out[2] = pixel >> (argb_order ? 0 : 16);
			out[3] = has_alpha ? pixel >> 24 : 255;
		}
	}

	return true;
}

// X11 CARDINAL ARGB is straight alpha; Flutter rawRgba requires premultiplied alpha.
inline std::vector<IconImage> parse_x11_icons(const uint32_t *data, size_t count) {
	std::vector<IconImage> images;
	size_t bytes = 0;
	for (size_t at = 0; at < count;) {
		if (count - at < 2)
			return {};

		const uint32_t w = data[at++], h = data[at++];
		const uint64_t pixels = uint64_t(w) * h;
		if (!w || !h || pixels > count - at)
			return {};

		if (valid_icon_size(w, h) && images.size() < icon_variant_limit &&
			pixels * 4 <= icon_byte_limit - bytes) {
			IconImage image{w, h, 1, {}};
			image.rgba.resize(size_t(pixels) * 4);

			for (size_t i = 0; i < pixels; ++i) {
				const uint32_t argb = data[at + i], alpha = argb >> 24;
				for (size_t c = 0; c < 3; ++c)
					image.rgba[i * 4 + c] = (((argb >> (16 - c * 8)) & 255) * alpha + 127) / 255;
				image.rgba[i * 4 + 3] = alpha;
			}

			bytes += image.rgba.size();
			images.push_back(std::move(image));
		}

		at += size_t(pixels);
	}

	return images;
}

} // namespace zenith
