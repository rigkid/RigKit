#include "FontCharsets.h"

#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CFontLayer.h"
#include "CName.h"
#include "CRelationship.h"
#include "FontFind.h"

#include <cmath>
#include <cstdio>
#include <unordered_set>

namespace rigkit {
namespace font {

namespace {

/// AGL names for U+0020..U+007E, indexed by cp - 0x20.
constexpr const char* kAsciiNames[] = {"space",		  "exclam",
									   "quotedbl",	  "numbersign",
									   "dollar",	  "percent",
									   "ampersand",	  "quotesingle",
									   "parenleft",	  "parenright",
									   "asterisk",	  "plus",
									   "comma",		  "hyphen",
									   "period",	  "slash",
									   "zero",		  "one",
									   "two",		  "three",
									   "four",		  "five",
									   "six",		  "seven",
									   "eight",		  "nine",
									   "colon",		  "semicolon",
									   "less",		  "equal",
									   "greater",	  "question",
									   "at",		  "A",
									   "B",			  "C",
									   "D",			  "E",
									   "F",			  "G",
									   "H",			  "I",
									   "J",			  "K",
									   "L",			  "M",
									   "N",			  "O",
									   "P",			  "Q",
									   "R",			  "S",
									   "T",			  "U",
									   "V",			  "W",
									   "X",			  "Y",
									   "Z",			  "bracketleft",
									   "backslash",	  "bracketright",
									   "asciicircum", "underscore",
									   "grave",		  "a",
									   "b",			  "c",
									   "d",			  "e",
									   "f",			  "g",
									   "h",			  "i",
									   "j",			  "k",
									   "l",			  "m",
									   "n",			  "o",
									   "p",			  "q",
									   "r",			  "s",
									   "t",			  "u",
									   "v",			  "w",
									   "x",			  "y",
									   "z",			  "braceleft",
									   "bar",		  "braceright",
									   "asciitilde"};

} // namespace

const std::vector<Charset>& charsets() {
	static const std::vector<Charset> kSets = {
		{"Latin Basic", 0x0020, 0x007E},
		{"Latin-1 Supplement", 0x00A0, 0x00FF},
		{"Latin Extended-A", 0x0100, 0x017F},
		{"Latin Extended-B", 0x0180, 0x024F},
	};
	return kSets;
}

int charsetIndex(uint32_t cp) {
	const auto& sets = charsets();
	for (size_t i = 0; i < sets.size(); ++i) {
		if (cp >= sets[i].first && cp <= sets[i].last) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

std::string glyphNameForCodepoint(uint32_t cp) {
	if (cp >= 0x20 && cp <= 0x7E) {
		return kAsciiNames[cp - 0x20];
	}
	char buf[16];
	std::snprintf(buf, sizeof(buf), cp > 0xFFFF ? "u%05X" : "uni%04X", cp);
	return buf;
}

int generateCharset(MEcs& ecs, const Charset& cs, entt::entity layer) {
	if (layer == entt::null) {
		for (auto e : ecs.registry().view<ecs::CFontLayer>()) {
			layer = e;
			if (ecs.registry().get<ecs::CFontLayer>(e).role == ecs::CFontLayer::Foreground) {
				break;
			}
		}
	}
	auto inLayer = [&](entt::entity e) {
		if (layer == entt::null) {
			return true;
		}
		return ecs.hasComponent<ecs::CRelationship>(e) &&
			   ecs.getComponent<ecs::CRelationship>(e).parent == layer;
	};
	std::unordered_set<uint32_t> have;
	for (auto e : ecs.registry().view<ecs::CFontGlyph>()) {
		if (!inLayer(e)) {
			continue;
		}
		for (uint32_t u : ecs.registry().get<ecs::CFontGlyph>(e).unicodes) {
			have.insert(u);
		}
	}
	float width = 500.f;
	for (auto e : ecs.registry().view<ecs::CFontFace>()) {
		width = std::round(ecs.registry().get<ecs::CFontFace>(e).unitsPerEm * 0.5f);
		break;
	}
	int made = 0;
	for (uint32_t cp = cs.first; cp <= cs.last; ++cp) {
		if (have.count(cp)) {
			continue;
		}
		const std::string name = glyphNameForCodepoint(cp);
		if (layer != entt::null ? (findGlyphInLayer(ecs, layer, name) != entt::null)
								: (findNamed(ecs, name) != entt::null)) {
			continue;
		}
		auto e = ecs.createEntity(name);
		ecs::CFontGlyph g;
		g.unicodes = {cp};
		g.width = width;
		ecs.addComponent(e, g);
		ecs::CName n;
		n.name = name;
		n.stableId = name;
		ecs.addComponent(e, n);
		if (layer != entt::null) {
			ecs::CRelationship rel;
			rel.parent = layer;
			ecs.addComponent(e, rel);
		}
		++made;
	}
	return made;
}

} // namespace font
} // namespace rigkit
