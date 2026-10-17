#include "core/util/AppIcon.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <utility>

namespace AppIcon {
namespace {

uint16_t readU16(const unsigned char* p) {
	return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

uint32_t readU32(const unsigned char* p) {
	return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
		   (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

// One BMP-encoded .ico entry: BITMAPINFOHEADER, optional palette, bottom-up
// pixel rows (stride padded to 4 bytes), then a 1 bpp AND transparency mask.
// Handles the uncompressed depths icons actually ship: 1 / 4 / 8 / 24 / 32 bpp.
bool decodeBmpEntry(const unsigned char* data, size_t size, IconImage& out) {
	if (size < 40) {
		return false;
	}
	const uint32_t headerSize = readU32(data);
	const int32_t width = static_cast<int32_t>(readU32(data + 4));
	// biHeight counts XOR + AND mask rows, so it is twice the visible height.
	const int32_t height = static_cast<int32_t>(readU32(data + 8)) / 2;
	const uint16_t bpp = readU16(data + 14);
	const uint32_t compression = readU32(data + 16);
	if (headerSize < 40 || headerSize > size || compression != 0 /* BI_RGB */ ||
		(bpp != 32 && bpp != 24 && bpp != 8 && bpp != 4 && bpp != 1) || width <= 0 || height <= 0 ||
		width > 1024 || height > 1024) {
		return false;
	}

	// Paletted depths carry BGR0 quads between the header and the pixel rows.
	uint32_t paletteCount = 0;
	if (bpp <= 8) {
		paletteCount = readU32(data + 32); // biClrUsed, 0 = full palette
		if (paletteCount == 0) {
			paletteCount = 1u << bpp;
		}
		if (paletteCount > 256) {
			return false;
		}
	}
	const unsigned char* palette = data + headerSize;
	const unsigned char* xorData = palette + static_cast<size_t>(paletteCount) * 4;

	const size_t xorStride = ((static_cast<size_t>(width) * bpp + 31) / 32) * 4;
	const size_t andStride = ((static_cast<size_t>(width) + 31) / 32) * 4;
	const size_t pixelOffset = headerSize + static_cast<size_t>(paletteCount) * 4;
	if (pixelOffset + xorStride * static_cast<size_t>(height) > size) {
		return false;
	}
	const unsigned char* andData = xorData + xorStride * static_cast<size_t>(height);
	const bool hasMask =
		pixelOffset + (xorStride + andStride) * static_cast<size_t>(height) <= size;

	out.width = width;
	out.height = height;
	out.rgba.assign(static_cast<size_t>(width) * height * 4, 0);
	bool anyAlpha = false;
	for (int y = 0; y < height; ++y) {
		const unsigned char* row = xorData + xorStride * static_cast<size_t>(height - 1 - y);
		for (int x = 0; x < width; ++x) {
			unsigned char* dst = out.rgba.data() + (static_cast<size_t>(y) * width + x) * 4;
			if (bpp >= 24) {
				const unsigned char* px = row + static_cast<size_t>(x) * (bpp / 8);
				dst[0] = px[2];
				dst[1] = px[1];
				dst[2] = px[0];
				dst[3] = bpp == 32 ? px[3] : 255;
				anyAlpha = anyAlpha || (bpp == 32 && px[3] != 0);
			} else {
				uint32_t idx = 0;
				if (bpp == 8) {
					idx = row[x];
				} else if (bpp == 4) {
					idx = (row[x / 2] >> ((1 - (x % 2)) * 4)) & 0xF;
				} else {
					idx = (row[x / 8] >> (7 - (x % 8))) & 1;
				}
				if (idx >= paletteCount) {
					idx = 0;
				}
				const unsigned char* pe = palette + static_cast<size_t>(idx) * 4;
				dst[0] = pe[2];
				dst[1] = pe[1];
				dst[2] = pe[0];
				dst[3] = 255;
			}
		}
	}
	// Transparency lives in the AND mask for everything except 32 bpp with a
	// real alpha channel (older 32 bpp writers also used the mask).
	if (hasMask && !anyAlpha) {
		for (int y = 0; y < height; ++y) {
			const unsigned char* row = andData + andStride * static_cast<size_t>(height - 1 - y);
			for (int x = 0; x < width; ++x) {
				const bool transparent = (row[x / 8] >> (7 - (x % 8))) & 1;
				out.rgba[(static_cast<size_t>(y) * width + x) * 4 + 3] = transparent ? 0 : 255;
			}
		}
	} else if (!hasMask && bpp == 32 && !anyAlpha) {
		// No mask and an unused alpha channel - treat as opaque, not invisible.
		for (size_t i = 3; i < out.rgba.size(); i += 4) {
			out.rgba[i] = 255;
		}
	}
	return true;
}

uint32_t fnv1a(const std::string& s) {
	uint32_t h = 2166136261u;
	for (unsigned char c : s) {
		h ^= c;
		h *= 16777619u;
	}
	return h;
}

// h in turns (1 = full circle), s and v in 0..1.
void hsvToRgb(float h, float s, float v, unsigned char rgb[3]) {
	h = h - std::floor(h);
	if (h < 0.f) {
		h += 1.f;
	}
	if (s < 0.f) {
		s = 0.f;
	} else if (s > 1.f) {
		s = 1.f;
	}
	if (v < 0.f) {
		v = 0.f;
	} else if (v > 1.f) {
		v = 1.f;
	}
	const float c = v * s;
	const float x = c * (1.f - std::fabs(std::fmod(h * 6.f, 2.f) - 1.f));
	const float m = v - c;
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
	int sector = static_cast<int>(h * 6.f);
	if (sector > 5) {
		sector = 5;
	}
	switch (sector) {
	case 0: r = c; g = x; break;
	case 1: r = x; g = c; break;
	case 2: g = c; b = x; break;
	case 3: g = x; b = c; break;
	case 4: r = x; b = c; break;
	default: r = c; b = x; break;
	}
	rgb[0] = static_cast<unsigned char>(std::lround((r + m) * 255.f));
	rgb[1] = static_cast<unsigned char>(std::lround((g + m) * 255.f));
	rgb[2] = static_cast<unsigned char>(std::lround((b + m) * 255.f));
}

// Name hash -> one hue. Background is that hue washed out and lifted.
// Pattern colours are the same hue, stepped around the wheel and kept darker.
void paletteFromName(uint32_t colorHash, unsigned char bg[3], unsigned char accents[4][3]) {
	const float hue = static_cast<float>(colorHash % 360u) / 360.f;
	const float sat = 0.55f + static_cast<float>((colorHash >> 9) & 31u) / 100.f;
	const float jitter = static_cast<float>((colorHash >> 16) & 15u) / 360.f;
	hsvToRgb(hue, sat * 0.22f, 0.94f, bg);
	const float turns[4] = {0.02f, 0.09f, 0.42f, 0.55f};
	const float values[4] = {0.74f, 0.50f, 0.60f, 0.36f};
	float accentSat = sat + 0.28f;
	if (accentSat > 1.f) {
		accentSat = 1.f;
	}
	for (int i = 0; i < 4; ++i) {
		hsvToRgb(hue + turns[i] + jitter, accentSat, values[i], accents[i]);
	}
}

// 5x5 pattern with a 1-cell border on a 7x7 logical grid, nearest-scaled.
// Each pattern row keeps one colour so the mirrored shape stays readable.
IconImage renderIdenticon(const bool cells[5][5], const unsigned char bg[3],
						  const unsigned char rowFg[5][3], int size) {
	IconImage img;
	img.width = size;
	img.height = size;
	img.rgba.resize(static_cast<size_t>(size) * size * 4);
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			const int cx = x * 7 / size - 1;
			const int cy = y * 7 / size - 1;
			const bool on = cx >= 0 && cx < 5 && cy >= 0 && cy < 5 && cells[cy][cx];
			const unsigned char* c = on ? rowFg[cy] : bg;
			unsigned char* dst = img.rgba.data() + (static_cast<size_t>(y) * size + x) * 4;
			dst[0] = c[0];
			dst[1] = c[1];
			dst[2] = c[2];
			dst[3] = 255;
		}
	}
	return img;
}

} // namespace

std::vector<IconImage> loadIco(const std::string& path) {
	std::vector<IconImage> icons;
	std::ifstream in(path, std::ios::binary);
	if (!in) {
		return icons;
	}
	const std::vector<unsigned char> file((std::istreambuf_iterator<char>(in)),
										  std::istreambuf_iterator<char>());
	if (file.size() < 6) {
		return icons;
	}
	const unsigned char* d = file.data();
	if (readU16(d) != 0 || readU16(d + 2) != 1) { // ICONDIR: reserved 0, type 1
		return icons;
	}
	const uint16_t count = readU16(d + 4);
	for (uint16_t i = 0; i < count; ++i) {
		const size_t entry = 6 + static_cast<size_t>(i) * 16;
		if (entry + 16 > file.size()) {
			break;
		}
		const uint32_t bytes = readU32(d + entry + 8);
		const uint32_t offset = readU32(d + entry + 12);
		if (bytes < 8 || offset > file.size() || bytes > file.size() - offset) {
			continue;
		}
		static const unsigned char pngSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
		if (std::memcmp(d + offset, pngSig, 8) == 0) {
			continue; // PNG entry - would need a PNG decoder
		}
		IconImage img;
		if (decodeBmpEntry(d + offset, bytes, img)) {
			icons.push_back(std::move(img));
		}
	}
	return icons;
}

bool writeIco(const std::string& path, const std::vector<IconImage>& icons) {
	if (icons.empty()) {
		return false;
	}
	for (const auto& img : icons) {
		if (img.width <= 0 || img.height <= 0 || img.width > 256 || img.height > 256 ||
			img.rgba.size() != static_cast<size_t>(img.width) * img.height * 4) {
			return false;
		}
	}

	auto appendU16 = [](std::vector<unsigned char>& out, uint16_t v) {
		out.push_back(static_cast<unsigned char>(v & 0xFF));
		out.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
	};
	auto appendU32 = [](std::vector<unsigned char>& out, uint32_t v) {
		out.push_back(static_cast<unsigned char>(v & 0xFF));
		out.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
		out.push_back(static_cast<unsigned char>((v >> 16) & 0xFF));
		out.push_back(static_cast<unsigned char>((v >> 24) & 0xFF));
	};

	const uint16_t count = static_cast<uint16_t>(icons.size());
	std::vector<std::vector<unsigned char>> entries;
	entries.reserve(icons.size());
	for (const auto& img : icons) {
		const size_t xorStride = static_cast<size_t>(img.width) * 4; // 32 bpp, already 4-aligned
		const size_t andStride = ((static_cast<size_t>(img.width) + 31) / 32) * 4;
		std::vector<unsigned char> entry;
		entry.reserve(40 + xorStride * img.height + andStride * img.height);
		appendU32(entry, 40);									 // biSize
		appendU32(entry, static_cast<uint32_t>(img.width));		 // biWidth
		appendU32(entry, static_cast<uint32_t>(img.height * 2)); // XOR + AND
		appendU16(entry, 1);									 // biPlanes
		appendU16(entry, 32);									 // biBitCount
		appendU32(entry, 0);									 // BI_RGB
		appendU32(entry, 0);									 // biSizeImage
		appendU32(entry, 0);									 // biXPelsPerMeter
		appendU32(entry, 0);									 // biYPelsPerMeter
		appendU32(entry, 0);									 // biClrUsed
		appendU32(entry, 0);									 // biClrImportant
		// Bottom-up BGRA; alpha from the source RGBA.
		for (int y = img.height - 1; y >= 0; --y) {
			for (int x = 0; x < img.width; ++x) {
				const unsigned char* src =
					img.rgba.data() + (static_cast<size_t>(y) * img.width + x) * 4;
				entry.push_back(src[2]);
				entry.push_back(src[1]);
				entry.push_back(src[0]);
				entry.push_back(src[3]);
			}
		}
		// Opaque AND mask (transparency is in the alpha channel).
		entry.resize(entry.size() + andStride * static_cast<size_t>(img.height), 0);
		entries.push_back(std::move(entry));
	}

	std::vector<unsigned char> file;
	appendU16(file, 0); // reserved
	appendU16(file, 1); // type = icon
	appendU16(file, count);
	uint32_t offset = 6 + static_cast<uint32_t>(count) * 16;
	for (size_t i = 0; i < icons.size(); ++i) {
		const int w = icons[i].width;
		const int h = icons[i].height;
		file.push_back(w >= 256 ? 0 : static_cast<unsigned char>(w));
		file.push_back(h >= 256 ? 0 : static_cast<unsigned char>(h));
		file.push_back(0);	 // color count
		file.push_back(0);	 // reserved
		appendU16(file, 1);	 // planes
		appendU16(file, 32); // bit count
		appendU32(file, static_cast<uint32_t>(entries[i].size()));
		appendU32(file, offset);
		offset += static_cast<uint32_t>(entries[i].size());
	}
	for (auto& entry : entries) {
		file.insert(file.end(), entry.begin(), entry.end());
	}

	std::ofstream out(path, std::ios::binary);
	if (!out) {
		return false;
	}
	out.write(reinterpret_cast<const char*>(file.data()),
			  static_cast<std::streamsize>(file.size()));
	return static_cast<bool>(out);
}

std::vector<IconImage> makeDefaultIcon(const std::string& appName) {
	const uint32_t hash = fnv1a(appName);

	const uint32_t colorHash = fnv1a(appName + "#palette");
	unsigned char bg[3];
	unsigned char accents[4][3];
	paletteFromName(colorHash, bg, accents);

	unsigned char rowFg[5][3];
	for (int y = 0; y < 5; ++y) {
		const unsigned char* src = accents[(colorHash >> (y * 2)) % 4u];
		rowFg[y][0] = src[0];
		rowFg[y][1] = src[1];
		rowFg[y][2] = src[2];
	}

	// 15 hash bits fill 3 columns x 5 rows, mirrored to 5x5.
	bool cells[5][5];
	for (int y = 0; y < 5; ++y) {
		for (int x = 0; x < 3; ++x) {
			const bool on = (hash >> (y * 3 + x)) & 1;
			cells[y][x] = on;
			cells[y][4 - x] = on;
		}
	}

	std::vector<IconImage> icons;
	for (int size : {16, 32, 48}) {
		icons.push_back(renderIdenticon(cells, bg, rowFg, size));
	}
	return icons;
}

} // namespace AppIcon

