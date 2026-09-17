#include "FontShape.h"

#include "CFontAvar.h"
#include "CFontAxis.h"
#include "CFontCell.h"
#include "CFontComponent.h"
#include "CFontGlyph.h"
#include "CName.h"
#include "CRelationship.h"
#include "FontFind.h"

#include <algorithm>
#include <cmath>

namespace rigkit {
namespace font {
namespace {

bool nextCodepoint(const char*& p, const char* end, uint32_t& cp) {
	if (p >= end) {
		return false;
	}
	const auto c = static_cast<unsigned char>(*p++);
	if (c < 0x80) {
		cp = c;
		return true;
	}
	int extra = 0;
	uint32_t minv = 0;
	if ((c & 0xE0) == 0xC0) {
		cp = c & 0x1F;
		extra = 1;
		minv = 0x80;
	} else if ((c & 0xF0) == 0xE0) {
		cp = c & 0x0F;
		extra = 2;
		minv = 0x800;
	} else if ((c & 0xF8) == 0xF0) {
		cp = c & 0x07;
		extra = 3;
		minv = 0x10000;
	} else {
		cp = 0xFFFD;
		return true;
	}
	for (int i = 0; i < extra; ++i) {
		if (p >= end) {
			cp = 0xFFFD;
			return true;
		}
		const auto n = static_cast<unsigned char>(*p++);
		if ((n & 0xC0) != 0x80) {
			cp = 0xFFFD;
			return true;
		}
		cp = (cp << 6) | (n & 0x3F);
	}
	if (cp < minv) {
		cp = 0xFFFD;
	}
	return true;
}

glm::vec2 xform(const ecs::CFontComponent& c, const glm::vec2& p) {
	return {c.scaleX * p.x + c.xyScale * p.y + c.offsetX,
			c.yxScale * p.x + c.scaleY * p.y + c.offsetY};
}

void appendTransformed(ecs::CPath& dst, const ecs::CPath& src, const ecs::CFontComponent& c) {
	for (auto cmd : src.commands) {
		cmd.p = xform(c, cmd.p);
		cmd.c1 = xform(c, cmd.c1);
		cmd.c2 = xform(c, cmd.c2);
		dst.commands.push_back(cmd);
	}
}

float mapAvar(const ecs::CFontAvar& avar, float n) {
	if (avar.segments.size() < 2) {
		return n;
	}
	const auto& segs = avar.segments;
	if (n <= segs.front().fromCoord) {
		return segs.front().toCoord;
	}
	if (n >= segs.back().fromCoord) {
		return segs.back().toCoord;
	}
	for (size_t i = 1; i < segs.size(); ++i) {
		const auto& a = segs[i - 1];
		const auto& b = segs[i];
		if (n <= b.fromCoord) {
			const float span = b.fromCoord - a.fromCoord;
			const float t = (span == 0.f) ? 0.f : (n - a.fromCoord) / span;
			return a.toCoord + t * (b.toCoord - a.toCoord);
		}
	}
	return n;
}

float normalizeAxis(const ecs::CFontAxis& ax, float design) {
	if (design >= ax.defaultValue) {
		const float span = ax.max - ax.defaultValue;
		return (span == 0.f) ? 0.f : (design - ax.defaultValue) / span;
	}
	const float span = ax.defaultValue - ax.min;
	return (span == 0.f) ? 0.f : (design - ax.defaultValue) / span;
}

float axisBlend(MEcs& ecs, const std::string& tag,
				const std::unordered_map<std::string, float>& location) {
	for (auto e : ecs.registry().view<ecs::CFontAxis>()) {
		const auto& ax = ecs.registry().get<ecs::CFontAxis>(e);
		if (ax.tag != tag) {
			continue;
		}
		float design = ax.defaultValue;
		if (auto it = location.find(tag); it != location.end()) {
			design = it->second;
		}
		float n = normalizeAxis(ax, design);
		if (ecs.hasComponent<ecs::CFontAvar>(e)) {
			n = mapAvar(ecs.getComponent<ecs::CFontAvar>(e), n);
		}
		return n;
	}
	if (auto it = location.find(tag); it != location.end()) {
		return it->second;
	}
	return 0.f;
}

float halfWeight(float blend, int sign) {
	if (sign >= 0) {
		return std::max(0.f, blend);
	}
	return std::max(0.f, -blend);
}

float tent(float n, float center, float wl, float wr) {
	if (n <= center) {
		if (wl <= 0.f) {
			return (n == center) ? 1.f : 0.f;
		}
		const float t = 1.f - (center - n) / wl;
		return std::clamp(t, 0.f, 1.f);
	}
	if (wr <= 0.f) {
		return (n == center) ? 1.f : 0.f;
	}
	const float t = 1.f - (n - center) / wr;
	return std::clamp(t, 0.f, 1.f);
}

float termWeight(MEcs& ecs, const ecs::CFontCell::Term& t,
				 const std::unordered_map<std::string, float>& location) {
	const float b1 = axisBlend(ecs, t.axisTag, location);
	float w = t.hasHat ? tent(b1, t.center, t.wl, t.wr) : halfWeight(b1, t.sign);
	if (!t.axisTag2.empty()) {
		w *= halfWeight(axisBlend(ecs, t.axisTag2, location), t.sign2);
	}
	if (!t.axisTag3.empty()) {
		w *= halfWeight(axisBlend(ecs, t.axisTag3, location), t.sign3);
	}
	return w;
}

void applyCell(ecs::CPath& path, float& advance, float weight, const ecs::CFontCell::Term& term) {
	if (weight == 0.f) {
		return;
	}
	advance += weight * term.adv;
	size_t cmdIndex = 0;
	for (auto& cmd : path.commands) {
		if (cmd.type == ecs::CPath::Cmd::Close) {
			continue;
		}
		if (cmdIndex >= term.segs.size()) {
			break;
		}
		const auto& seg = term.segs[cmdIndex];
		if (!seg.points.empty()) {
			cmd.p += weight * seg.points[0];
		}
		++cmdIndex;
	}
}

void appendComponents(MEcs& ecs, entt::entity glyph, ecs::CPath& path,
					  const std::unordered_map<std::string, float>& location) {
	for (auto e : ecs.registry().view<ecs::CRelationship, ecs::CFontComponent>()) {
		if (ecs.registry().get<ecs::CRelationship>(e).parent != glyph) {
			continue;
		}
		const auto& comp = ecs.registry().get<ecs::CFontComponent>(e);
		auto src = findNamed(ecs, comp.glyph);
		if (src == entt::null) {
			continue;
		}
		ecs::CPath child;
		float unused = 0.f;
		if (!reconstructGlyph(ecs, src, location, child, unused)) {
			continue;
		}
		appendTransformed(path, child, comp);
	}
}

} // namespace

bool reconstructGlyph(MEcs& ecs, entt::entity glyph,
					  const std::unordered_map<std::string, float>& location, ecs::CPath& outPath,
					  float& outAdvance) {
	if (glyph == entt::null || !ecs.hasComponent<ecs::CFontGlyph>(glyph)) {
		return false;
	}
	outAdvance = ecs.getComponent<ecs::CFontGlyph>(glyph).width;
	outPath = {};
	if (ecs.hasComponent<ecs::CPath>(glyph)) {
		outPath = ecs.getComponent<ecs::CPath>(glyph);
	}
	if (ecs.hasComponent<ecs::CFontCell>(glyph)) {
		const auto& cell = ecs.getComponent<ecs::CFontCell>(glyph);
		if (cell.baseAdv != 0.f) {
			outAdvance = cell.baseAdv;
		}
		for (const auto& term : cell.terms) {
			applyCell(outPath, outAdvance, termWeight(ecs, term, location), term);
		}
	}
	appendComponents(ecs, glyph, outPath, location);
	return !outPath.empty() || ecs.hasComponent<ecs::CFontGlyph>(glyph);
}

ShapedRun shapeString(MEcs& ecs, std::string_view text, bool applyKern,
					  const std::unordered_map<std::string, float>& location) {
	ShapedRun run;
	const char* p = text.data();
	const char* end = p + text.size();
	std::string prev;
	float pen = 0.f;
	uint32_t cp = 0;
	while (nextCodepoint(p, end, cp)) {
		auto e = findGlyphByUnicode(ecs, cp);
		if (e == entt::null) {
			continue;
		}
		ShapedGlyph g;
		g.entity = e;
		g.name = stableName(ecs, e);
		if (applyKern && !prev.empty()) {
			pen += kernValue(ecs, prev, g.name);
		}
		g.x = pen;
		if (!reconstructGlyph(ecs, e, location, g.path, g.advance)) {
			g.advance = ecs.getComponent<ecs::CFontGlyph>(e).width;
		}
		pen += g.advance;
		prev = g.name;
		run.glyphs.push_back(std::move(g));
	}
	run.width = pen;
	return run;
}

bool writeCellFromPathDiff(MEcs& ecs, entt::entity glyph, const ecs::CPath& other,
						   const std::string& axisTag, int sign) {
	if (glyph == entt::null || !ecs.hasComponent<ecs::CPath>(glyph) || axisTag.empty()) {
		return false;
	}
	const auto& base = ecs.getComponent<ecs::CPath>(glyph);
	if (base.commands.size() != other.commands.size()) {
		return false;
	}
	ecs::CFontCell::Term term;
	term.axisTag = axisTag;
	term.sign = sign;
	for (size_t i = 0; i < base.commands.size(); ++i) {
		if (base.commands[i].type != other.commands[i].type) {
			return false;
		}
		if (base.commands[i].type == ecs::CPath::Cmd::Close) {
			continue;
		}
		ecs::CFontCell::DeltaSeg seg;
		seg.type = ecs::CFontCell::DeltaSeg::Line;
		seg.points.push_back(other.commands[i].p - base.commands[i].p);
		term.segs.push_back(std::move(seg));
	}
	ecs::CFontCell cell;
	if (ecs.hasComponent<ecs::CFontCell>(glyph)) {
		cell = ecs.getComponent<ecs::CFontCell>(glyph);
	} else if (ecs.hasComponent<ecs::CFontGlyph>(glyph)) {
		cell.baseAdv = ecs.getComponent<ecs::CFontGlyph>(glyph).width;
	}
	cell.terms.push_back(std::move(term));
	if (ecs.hasComponent<ecs::CFontCell>(glyph)) {
		ecs.getComponent<ecs::CFontCell>(glyph) = cell;
	} else {
		ecs.addComponent(glyph, cell);
	}
	return true;
}

} // namespace font
} // namespace rigkit
