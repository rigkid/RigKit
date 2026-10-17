#include "PsdFile.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace rigkit {
namespace psd {
namespace {

bool readU8(std::istream& in, uint8_t& v) {
	return static_cast<bool>(in.read(reinterpret_cast<char*>(&v), 1));
}

bool readI16(std::istream& in, int16_t& v) {
	uint8_t b[2];
	if (!in.read(reinterpret_cast<char*>(b), 2)) {
		return false;
	}
	v = static_cast<int16_t>((b[0] << 8) | b[1]);
	return true;
}

bool readI32(std::istream& in, int32_t& v) {
	uint8_t b[4];
	if (!in.read(reinterpret_cast<char*>(b), 4)) {
		return false;
	}
	v = static_cast<int32_t>((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
	return true;
}

bool readU32(std::istream& in, uint32_t& v) {
	int32_t s = 0;
	if (!readI32(in, s)) {
		return false;
	}
	v = static_cast<uint32_t>(s);
	return true;
}

bool skip(std::istream& in, uint32_t n) {
	return n == 0 || static_cast<bool>(in.seekg(static_cast<std::streamoff>(n), std::ios::cur));
}

void writeU8(std::ostream& out, uint8_t v) {
	out.write(reinterpret_cast<const char*>(&v), 1);
}

void writeI16(std::ostream& out, int16_t v) {
	const uint8_t b[2] = {static_cast<uint8_t>((v >> 8) & 0xff), static_cast<uint8_t>(v & 0xff)};
	out.write(reinterpret_cast<const char*>(b), 2);
}

void writeI32(std::ostream& out, int32_t v) {
	const uint8_t b[4] = {static_cast<uint8_t>((v >> 24) & 0xff),
						  static_cast<uint8_t>((v >> 16) & 0xff),
						  static_cast<uint8_t>((v >> 8) & 0xff), static_cast<uint8_t>(v & 0xff)};
	out.write(reinterpret_cast<const char*>(b), 4);
}

void writeU32(std::ostream& out, uint32_t v) {
	writeI32(out, static_cast<int32_t>(v));
}

bool decodeRawChannel(std::istream& in, int w, int h, std::vector<uint8_t>& plane) {
	const size_t n = static_cast<size_t>(std::max(0, w) * std::max(0, h));
	plane.resize(n);
	return n == 0 || static_cast<bool>(in.read(reinterpret_cast<char*>(plane.data()),
											   static_cast<std::streamsize>(n)));
}

bool decodeRleChannel(std::istream& in, int w, int h, std::vector<uint8_t>& plane) {
	if (w <= 0 || h <= 0) {
		plane.clear();
		return true;
	}
	std::vector<int16_t> rowBytes(static_cast<size_t>(h));
	for (int y = 0; y < h; ++y) {
		if (!readI16(in, rowBytes[static_cast<size_t>(y)])) {
			return false;
		}
	}
	plane.assign(static_cast<size_t>(w * h), 0);
	for (int y = 0; y < h; ++y) {
		const int want = std::max(0, static_cast<int>(rowBytes[static_cast<size_t>(y)]));
		std::vector<uint8_t> packed(static_cast<size_t>(want));
		if (want > 0 &&
			!in.read(reinterpret_cast<char*>(packed.data()), static_cast<std::streamsize>(want))) {
			return false;
		}
		int x = 0;
		size_t i = 0;
		uint8_t* dest = plane.data() + static_cast<size_t>(y * w);
		while (x < w && i < packed.size()) {
			const int8_t n = static_cast<int8_t>(packed[i++]);
			if (n >= 0) {
				const int count = std::min(static_cast<int>(n) + 1, w - x);
				for (int k = 0; k < count && i < packed.size(); ++k) {
					dest[x++] = packed[i++];
				}
			} else if (n != -128) {
				if (i >= packed.size()) {
					break;
				}
				const uint8_t v = packed[i++];
				const int count = std::min(1 - static_cast<int>(n), w - x);
				for (int k = 0; k < count; ++k) {
					dest[x++] = v;
				}
			}
		}
	}
	return true;
}

void packRgba(int w, int h, const std::vector<uint8_t>& r, const std::vector<uint8_t>& g,
			  const std::vector<uint8_t>& b, const std::vector<uint8_t>& a,
			  std::vector<uint8_t>& out) {
	out.assign(static_cast<size_t>(w * h * 4), 255);
	const size_t n = static_cast<size_t>(w * h);
	for (size_t i = 0; i < n; ++i) {
		out[i * 4 + 0] = i < r.size() ? r[i] : 0;
		out[i * 4 + 1] = i < g.size() ? g[i] : 0;
		out[i * 4 + 2] = i < b.size() ? b[i] : 0;
		out[i * 4 + 3] = i < a.size() ? a[i] : 255;
	}
}

} // namespace

bool readFile(const std::string& path, PsdFile& out) {
	out = {};
	std::ifstream in(path, std::ios::binary);
	if (!in) {
		return false;
	}
	char sig[4] = {};
	if (!in.read(sig, 4) || std::memcmp(sig, "8BPS", 4) != 0) {
		return false;
	}
	int16_t version = 0;
	if (!readI16(in, version) || (version != 1 && version != 2)) {
		return false;
	}
	if (!skip(in, 6)) {
		return false;
	}
	int16_t channels = 0;
	int32_t height = 0;
	int32_t width = 0;
	int16_t depth = 0;
	int16_t mode = 0;
	if (!readI16(in, channels) || !readI32(in, height) || !readI32(in, width) ||
		!readI16(in, depth) || !readI16(in, mode)) {
		return false;
	}
	if (depth != 8 || mode != 3 || width <= 0 || height <= 0) {
		return false;
	}
	out.width = width;
	out.height = height;
	uint32_t colorLen = 0;
	if (!readU32(in, colorLen) || !skip(in, colorLen)) {
		return false;
	}
	uint32_t resLen = 0;
	if (!readU32(in, resLen) || !skip(in, resLen)) {
		return false;
	}
	uint32_t layerMaskLen = 0;
	if (!readU32(in, layerMaskLen)) {
		return false;
	}
	const std::streampos layerEnd = in.tellg() + static_cast<std::streamoff>(layerMaskLen);
	int32_t layerInfoLen = 0;
	if (layerMaskLen >= 4 && !readI32(in, layerInfoLen)) {
		return false;
	}
	int16_t layerCountSigned = 0;
	if (layerInfoLen != 0 && !readI16(in, layerCountSigned)) {
		return false;
	}
	const int layerCount = std::abs(static_cast<int>(layerCountSigned));
	struct Rec {
		int top = 0, left = 0, bottom = 0, right = 0;
		int chCount = 0;
		std::vector<int16_t> chId;
		std::vector<uint32_t> chLen;
		uint8_t opacity = 255;
		uint8_t flags = 0;
		std::string name;
	};
	std::vector<Rec> recs;
	recs.reserve(static_cast<size_t>(layerCount));
	for (int i = 0; i < layerCount; ++i) {
		Rec rec;
		if (!readI32(in, rec.top) || !readI32(in, rec.left) || !readI32(in, rec.bottom) ||
			!readI32(in, rec.right)) {
			return false;
		}
		int16_t ch = 0;
		if (!readI16(in, ch)) {
			return false;
		}
		rec.chCount = ch;
		rec.chId.resize(static_cast<size_t>(std::max(0, rec.chCount)));
		rec.chLen.resize(rec.chId.size());
		for (int c = 0; c < rec.chCount; ++c) {
			if (!readI16(in, rec.chId[static_cast<size_t>(c)]) ||
				!readU32(in, rec.chLen[static_cast<size_t>(c)])) {
				return false;
			}
		}
		char blend[8] = {};
		if (!in.read(blend, 8)) {
			return false;
		}
		if (!readU8(in, rec.opacity)) {
			return false;
		}
		uint8_t clipping = 0;
		if (!readU8(in, clipping) || !readU8(in, rec.flags)) {
			return false;
		}
		uint8_t filler = 0;
		if (!readU8(in, filler)) {
			return false;
		}
		uint32_t extra = 0;
		if (!readU32(in, extra)) {
			return false;
		}
		const std::streampos extraEnd = in.tellg() + static_cast<std::streamoff>(extra);
		uint32_t maskLen = 0;
		if (!readU32(in, maskLen) || !skip(in, maskLen)) {
			return false;
		}
		uint32_t blendRange = 0;
		if (!readU32(in, blendRange) || !skip(in, blendRange)) {
			return false;
		}
		uint8_t nameLen = 0;
		if (!readU8(in, nameLen)) {
			return false;
		}
		rec.name.assign(static_cast<size_t>(nameLen), '\0');
		if (nameLen > 0 && !in.read(rec.name.data(), static_cast<std::streamsize>(nameLen))) {
			return false;
		}
		const int pad = (4 - ((1 + nameLen) % 4)) % 4;
		if (!skip(in, static_cast<uint32_t>(pad))) {
			return false;
		}
		const auto here = in.tellg();
		if (here < extraEnd) {
			if (!skip(in, static_cast<uint32_t>(extraEnd - here))) {
				return false;
			}
		}
		recs.push_back(std::move(rec));
	}
	for (auto& rec : recs) {
		const int w = std::max(0, rec.right - rec.left);
		const int h = std::max(0, rec.bottom - rec.top);
		std::vector<uint8_t> r, g, b, a;
		for (int c = 0; c < rec.chCount; ++c) {
			int16_t comp = 0;
			if (!readI16(in, comp)) {
				return false;
			}
			std::vector<uint8_t> plane;
			const bool ok =
				(comp == 1) ? decodeRleChannel(in, w, h, plane) : decodeRawChannel(in, w, h, plane);
			if (!ok) {
				return false;
			}
			const int16_t id = rec.chId[static_cast<size_t>(c)];
			if (id == 0) {
				r = std::move(plane);
			} else if (id == 1) {
				g = std::move(plane);
			} else if (id == 2) {
				b = std::move(plane);
			} else if (id == -1) {
				a = std::move(plane);
			}
		}
		PsdLayer layer;
		layer.name = rec.name.empty() ? "Layer" : rec.name;
		layer.visible = (rec.flags & 0x02) == 0;
		layer.opacity = rec.opacity / 255.f;
		layer.left = rec.left;
		layer.top = rec.top;
		layer.w = w;
		layer.h = h;
		packRgba(w, h, r, g, b, a, layer.rgba);
		out.layers.push_back(std::move(layer));
	}
	if (in.tellg() < layerEnd) {
		in.seekg(layerEnd);
	}
	(void)channels;
	return out.width > 0 && out.height > 0;
}

bool writeFile(const std::string& path, const PsdFile& file) {
	if (file.width <= 0 || file.height <= 0) {
		return false;
	}
	std::ofstream out(path, std::ios::binary);
	if (!out) {
		return false;
	}
	out.write("8BPS", 4);
	writeI16(out, 1);
	const uint8_t reserved[6] = {};
	out.write(reinterpret_cast<const char*>(reserved), 6);
	writeI16(out, 4);
	writeI32(out, file.height);
	writeI32(out, file.width);
	writeI16(out, 8);
	writeI16(out, 3);
	writeU32(out, 0);
	writeU32(out, 0);

	const int n = static_cast<int>(file.layers.size());
	std::vector<std::vector<uint8_t>> r(static_cast<size_t>(n));
	std::vector<std::vector<uint8_t>> g(static_cast<size_t>(n));
	std::vector<std::vector<uint8_t>> b(static_cast<size_t>(n));
	std::vector<std::vector<uint8_t>> a(static_cast<size_t>(n));
	for (int i = 0; i < n; ++i) {
		const auto& L = file.layers[static_cast<size_t>(i)];
		const int w = std::max(0, L.w);
		const int h = std::max(0, L.h);
		const size_t px = static_cast<size_t>(w * h);
		r[static_cast<size_t>(i)].assign(px, 0);
		g[static_cast<size_t>(i)].assign(px, 0);
		b[static_cast<size_t>(i)].assign(px, 0);
		a[static_cast<size_t>(i)].assign(px, 255);
		for (size_t p = 0; p < px && (p * 4 + 3) < L.rgba.size(); ++p) {
			r[static_cast<size_t>(i)][p] = L.rgba[p * 4 + 0];
			g[static_cast<size_t>(i)][p] = L.rgba[p * 4 + 1];
			b[static_cast<size_t>(i)][p] = L.rgba[p * 4 + 2];
			a[static_cast<size_t>(i)][p] = L.rgba[p * 4 + 3];
		}
	}

	auto nameBytes = [](const std::string& name) {
		const uint8_t len = static_cast<uint8_t>(std::min<size_t>(name.size(), 255));
		const int pad = (4 - ((1 + len) % 4)) % 4;
		std::vector<uint8_t> bytes;
		bytes.push_back(len);
		bytes.insert(bytes.end(), name.begin(), name.begin() + len);
		bytes.insert(bytes.end(), static_cast<size_t>(pad), 0);
		return bytes;
	};

	int32_t layerInfo = 2; // layer count
	for (int i = 0; i < n; ++i) {
		const auto names = nameBytes(file.layers[static_cast<size_t>(i)].name);
		const int w = std::max(0, file.layers[static_cast<size_t>(i)].w);
		const int h = std::max(0, file.layers[static_cast<size_t>(i)].h);
		const uint32_t chBytes = static_cast<uint32_t>(w * h + 2);
		layerInfo += 16 + 2 + 4 * 6 + 8 + 4 + 4 + 4 + 4 + static_cast<int>(names.size());
		layerInfo += static_cast<int>(4 * chBytes);
	}
	if (layerInfo & 1) {
		++layerInfo;
	}
	writeU32(out, static_cast<uint32_t>(layerInfo + 4));
	writeI32(out, layerInfo);
	writeI16(out, static_cast<int16_t>(-n));
	for (int i = 0; i < n; ++i) {
		const auto& L = file.layers[static_cast<size_t>(i)];
		writeI32(out, L.top);
		writeI32(out, L.left);
		writeI32(out, L.top + L.h);
		writeI32(out, L.left + L.w);
		writeI16(out, 4);
		const uint32_t chBytes = static_cast<uint32_t>(std::max(0, L.w) * std::max(0, L.h) + 2);
		const int16_t ids[4] = {-1, 0, 1, 2};
		for (int16_t id : ids) {
			writeI16(out, id);
			writeU32(out, chBytes);
		}
		out.write("8BIMnorm", 8);
		writeU8(out, static_cast<uint8_t>(std::clamp(L.opacity, 0.f, 1.f) * 255.f));
		writeU8(out, 0);
		writeU8(out, L.visible ? 0 : 2);
		writeU8(out, 0);
		const auto names = nameBytes(L.name);
		writeU32(out, static_cast<uint32_t>(8 + names.size()));
		writeU32(out, 0);
		writeU32(out, 0);
		out.write(reinterpret_cast<const char*>(names.data()),
				  static_cast<std::streamsize>(names.size()));
	}
	auto writePlane = [&](const std::vector<uint8_t>& plane) {
		writeI16(out, 0);
		if (!plane.empty()) {
			out.write(reinterpret_cast<const char*>(plane.data()),
					  static_cast<std::streamsize>(plane.size()));
		}
	};
	for (int i = 0; i < n; ++i) {
		writePlane(a[static_cast<size_t>(i)]);
		writePlane(r[static_cast<size_t>(i)]);
		writePlane(g[static_cast<size_t>(i)]);
		writePlane(b[static_cast<size_t>(i)]);
	}
	if (layerInfo & 1) {
		writeU8(out, 0);
	}

	writeI16(out, 0);
	const size_t merged = static_cast<size_t>(file.width * file.height);
	std::vector<uint8_t> z(merged, 0);
	out.write(reinterpret_cast<const char*>(z.data()), static_cast<std::streamsize>(merged));
	out.write(reinterpret_cast<const char*>(z.data()), static_cast<std::streamsize>(merged));
	out.write(reinterpret_cast<const char*>(z.data()), static_cast<std::streamsize>(merged));
	out.write(reinterpret_cast<const char*>(z.data()), static_cast<std::streamsize>(merged));
	return static_cast<bool>(out);
}

} // namespace psd
} // namespace rigkit
