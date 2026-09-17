#include "FontSerializers.h"

#include "core/TypeJson.h"
#include "AddSerializer.h"
#include "CFontAnchor.h"
#include "CFontAvar.h"
#include "CFontAxis.h"
#include "CFontCell.h"
#include "CFontComponent.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CFontGroup.h"
#include "CFontKern.h"
#include "CFontLayer.h"

namespace rigkit {
namespace font {
namespace {

using project::ordered_json;

const char* roleId(int role) {
	if (role == ecs::CFontLayer::Background) {
		return "background";
	}
	if (role == ecs::CFontLayer::Spare) {
		return "spare";
	}
	return "foreground";
}

int roleFromId(const std::string& id) {
	if (id == "background") {
		return ecs::CFontLayer::Background;
	}
	if (id == "spare") {
		return ecs::CFontLayer::Spare;
	}
	return ecs::CFontLayer::Foreground;
}

const char* sideId(int side) {
	if (side == ecs::CFontGroup::Left) {
		return "left";
	}
	if (side == ecs::CFontGroup::Right) {
		return "right";
	}
	return nullptr;
}

int sideFromId(const std::string& id) {
	if (id == "left") {
		return ecs::CFontGroup::Left;
	}
	if (id == "right") {
		return ecs::CFontGroup::Right;
	}
	return ecs::CFontGroup::Unclassified;
}

bool serializeFace(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontFace>(e)) {
		return false;
	}
	const auto& f = reg.get<ecs::CFontFace>(e);
	if (!f.family.empty()) {
		j["family"] = f.family;
	}
	if (!f.style.empty()) {
		j["style"] = f.style;
	}
	j["unitsPerEm"] = f.unitsPerEm;
	j["ascender"] = f.ascender;
	j["descender"] = f.descender;
	j["capHeight"] = f.capHeight;
	j["xHeight"] = f.xHeight;
	if (!f.version.empty()) {
		j["version"] = f.version;
	}
	if (!f.features.empty()) {
		j["features"] = f.features;
	}
	return true;
}

bool deserializeFace(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontFace f;
	if (reg.all_of<ecs::CFontFace>(e)) {
		f = reg.get<ecs::CFontFace>(e);
	}
	f.family = j.value("family", f.family);
	f.style = j.value("style", f.style);
	f.unitsPerEm = j.value("unitsPerEm", f.unitsPerEm);
	f.ascender = j.value("ascender", f.ascender);
	f.descender = j.value("descender", f.descender);
	f.capHeight = j.value("capHeight", f.capHeight);
	f.xHeight = j.value("xHeight", f.xHeight);
	f.version = j.value("version", f.version);
	f.features = j.value("features", f.features);
	reg.emplace_or_replace<ecs::CFontFace>(e, std::move(f));
	return true;
}

bool serializeLayer(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontLayer>(e)) {
		return false;
	}
	j["role"] = roleId(reg.get<ecs::CFontLayer>(e).role);
	return true;
}

bool deserializeLayer(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontLayer layer;
	if (j.contains("role") && j["role"].is_string()) {
		layer.role = roleFromId(j["role"].get<std::string>());
	}
	reg.emplace_or_replace<ecs::CFontLayer>(e, layer);
	return true;
}

bool serializeGlyph(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontGlyph>(e)) {
		return false;
	}
	const auto& g = reg.get<ecs::CFontGlyph>(e);
	if (!g.unicodes.empty()) {
		auto arr = ordered_json::array();
		for (uint32_t u : g.unicodes) {
			arr.push_back(u);
		}
		j["unicodes"] = std::move(arr);
	}
	j["width"] = g.width;
	return true;
}

bool deserializeGlyph(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontGlyph g;
	if (reg.all_of<ecs::CFontGlyph>(e)) {
		g = reg.get<ecs::CFontGlyph>(e);
	}
	g.width = j.value("width", g.width);
	g.unicodes.clear();
	if (j.contains("unicodes") && j["unicodes"].is_array()) {
		for (const auto& u : j["unicodes"]) {
			if (u.is_number_unsigned() || u.is_number_integer()) {
				g.unicodes.push_back(u.get<uint32_t>());
			}
		}
	}
	reg.emplace_or_replace<ecs::CFontGlyph>(e, std::move(g));
	return true;
}

bool serializeComponent(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontComponent>(e)) {
		return false;
	}
	const auto& c = reg.get<ecs::CFontComponent>(e);
	j["glyph"] = c.glyph;
	if (c.scaleX != 1.f) {
		j["scaleX"] = c.scaleX;
	}
	if (c.xyScale != 0.f) {
		j["xyScale"] = c.xyScale;
	}
	if (c.yxScale != 0.f) {
		j["yxScale"] = c.yxScale;
	}
	if (c.scaleY != 1.f) {
		j["scaleY"] = c.scaleY;
	}
	if (c.offsetX != 0.f) {
		j["offsetX"] = c.offsetX;
	}
	if (c.offsetY != 0.f) {
		j["offsetY"] = c.offsetY;
	}
	return true;
}

bool deserializeComponent(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontComponent c;
	c.glyph = j.value("glyph", c.glyph);
	c.scaleX = j.value("scaleX", c.scaleX);
	c.xyScale = j.value("xyScale", c.xyScale);
	c.yxScale = j.value("yxScale", c.yxScale);
	c.scaleY = j.value("scaleY", c.scaleY);
	c.offsetX = j.value("offsetX", c.offsetX);
	c.offsetY = j.value("offsetY", c.offsetY);
	reg.emplace_or_replace<ecs::CFontComponent>(e, std::move(c));
	return true;
}

bool serializeAnchor(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontAnchor>(e)) {
		return false;
	}
	j["point"] = vec2ToJson(reg.get<ecs::CFontAnchor>(e).point);
	return true;
}

bool deserializeAnchor(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontAnchor a;
	if (j.contains("point")) {
		a.point = vec2FromJson(j["point"]);
	}
	reg.emplace_or_replace<ecs::CFontAnchor>(e, a);
	return true;
}

bool serializeKern(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontKern>(e)) {
		return false;
	}
	const auto& k = reg.get<ecs::CFontKern>(e);
	j["left"] = k.left;
	j["right"] = k.right;
	j["value"] = k.value;
	return true;
}

bool deserializeKern(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontKern k;
	k.left = j.value("left", k.left);
	k.right = j.value("right", k.right);
	k.value = j.value("value", k.value);
	reg.emplace_or_replace<ecs::CFontKern>(e, std::move(k));
	return true;
}

bool serializeGroup(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontGroup>(e)) {
		return false;
	}
	const auto& g = reg.get<ecs::CFontGroup>(e);
	auto members = ordered_json::array();
	for (const auto& m : g.members) {
		members.push_back(m);
	}
	j["members"] = std::move(members);
	if (const char* side = sideId(g.side)) {
		j["side"] = side;
	}
	return true;
}

bool deserializeGroup(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontGroup g;
	if (j.contains("members") && j["members"].is_array()) {
		for (const auto& m : j["members"]) {
			if (m.is_string()) {
				g.members.push_back(m.get<std::string>());
			}
		}
	}
	if (j.contains("side") && j["side"].is_string()) {
		g.side = sideFromId(j["side"].get<std::string>());
	}
	reg.emplace_or_replace<ecs::CFontGroup>(e, std::move(g));
	return true;
}

bool serializeAxis(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontAxis>(e)) {
		return false;
	}
	const auto& a = reg.get<ecs::CFontAxis>(e);
	j["tag"] = a.tag;
	if (!a.name.empty()) {
		j["name"] = a.name;
	}
	j["min"] = a.min;
	j["default"] = a.defaultValue;
	j["max"] = a.max;
	return true;
}

bool deserializeAxis(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontAxis a;
	if (reg.all_of<ecs::CFontAxis>(e)) {
		a = reg.get<ecs::CFontAxis>(e);
	}
	a.tag = j.value("tag", a.tag);
	a.name = j.value("name", a.name);
	a.min = j.value("min", a.min);
	a.defaultValue = j.value("default", a.defaultValue);
	a.max = j.value("max", a.max);
	reg.emplace_or_replace<ecs::CFontAxis>(e, std::move(a));
	return true;
}

bool serializeAvar(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontAvar>(e)) {
		return false;
	}
	auto segs = ordered_json::array();
	for (const auto& s : reg.get<ecs::CFontAvar>(e).segments) {
		ordered_json row;
		row["fromCoord"] = s.fromCoord;
		row["toCoord"] = s.toCoord;
		segs.push_back(std::move(row));
	}
	j["segments"] = std::move(segs);
	return true;
}

bool deserializeAvar(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontAvar a;
	if (j.contains("segments") && j["segments"].is_array()) {
		for (const auto& row : j["segments"]) {
			ecs::CFontAvar::Segment s;
			s.fromCoord = row.value("fromCoord", 0.f);
			s.toCoord = row.value("toCoord", 0.f);
			a.segments.push_back(s);
		}
	}
	reg.emplace_or_replace<ecs::CFontAvar>(e, std::move(a));
	return true;
}

bool serializeCell(entt::registry& reg, entt::entity e, ordered_json& j) {
	if (!reg.all_of<ecs::CFontCell>(e)) {
		return false;
	}
	const auto& c = reg.get<ecs::CFontCell>(e);
	j["baseAdv"] = c.baseAdv;
	auto terms = ordered_json::array();
	for (const auto& t : c.terms) {
		ordered_json tj;
		tj["axisTag"] = t.axisTag;
		tj["sign"] = t.sign;
		if (!t.axisTag2.empty()) {
			tj["axisTag2"] = t.axisTag2;
			tj["sign2"] = t.sign2;
		}
		if (!t.axisTag3.empty()) {
			tj["axisTag3"] = t.axisTag3;
			tj["sign3"] = t.sign3;
		}
		if (t.hasHat) {
			tj["center"] = t.center;
			tj["wl"] = t.wl;
			tj["wr"] = t.wr;
		}
		tj["adv"] = t.adv;
		auto segs = ordered_json::array();
		for (const auto& seg : t.segs) {
			ordered_json sj;
			sj["type"] = (seg.type == ecs::CFontCell::DeltaSeg::Quad) ? "quad" : "line";
			auto pts = ordered_json::array();
			for (const auto& p : seg.points) {
				pts.push_back(vec2ToJson(p));
			}
			sj["points"] = std::move(pts);
			segs.push_back(std::move(sj));
		}
		tj["segs"] = std::move(segs);
		terms.push_back(std::move(tj));
	}
	j["terms"] = std::move(terms);
	return true;
}

bool deserializeCell(entt::registry& reg, entt::entity e, const ordered_json& j) {
	ecs::CFontCell c;
	c.baseAdv = j.value("baseAdv", 0.f);
	if (j.contains("terms") && j["terms"].is_array()) {
		for (const auto& tj : j["terms"]) {
			ecs::CFontCell::Term t;
			t.axisTag = tj.value("axisTag", "");
			t.sign = tj.value("sign", 1);
			t.axisTag2 = tj.value("axisTag2", "");
			t.sign2 = tj.value("sign2", 1);
			t.axisTag3 = tj.value("axisTag3", "");
			t.sign3 = tj.value("sign3", 1);
			if (tj.contains("center") || tj.contains("wl") || tj.contains("wr")) {
				t.hasHat = true;
				t.center = tj.value("center", 0.f);
				t.wl = tj.value("wl", 0.f);
				t.wr = tj.value("wr", 0.f);
			}
			t.adv = tj.value("adv", 0.f);
			if (tj.contains("segs") && tj["segs"].is_array()) {
				for (const auto& sj : tj["segs"]) {
					ecs::CFontCell::DeltaSeg seg;
					if (sj.contains("type") && sj["type"].is_string() &&
						sj["type"].get<std::string>() == "quad") {
						seg.type = ecs::CFontCell::DeltaSeg::Quad;
					}
					if (sj.contains("points") && sj["points"].is_array()) {
						for (const auto& p : sj["points"]) {
							seg.points.push_back(vec2FromJson(p));
						}
					}
					t.segs.push_back(std::move(seg));
				}
			}
			c.terms.push_back(std::move(t));
		}
	}
	reg.emplace_or_replace<ecs::CFontCell>(e, std::move(c));
	return true;
}

} // namespace

void registerProjectSerializers(project::ComponentSerializerRegistry& registry) {
	project::addSerializer<ecs::CFontFace>(registry, "FontFace", "rig.font.face", serializeFace,
										   deserializeFace);
	project::addSerializer<ecs::CFontLayer>(registry, "FontLayer", "rig.font.layer", serializeLayer,
											deserializeLayer);
	project::addSerializer<ecs::CFontGlyph>(registry, "FontGlyph", "rig.font.glyph", serializeGlyph,
											deserializeGlyph);
	project::addSerializer<ecs::CFontComponent>(registry, "FontComponent", "rig.font.component",
												serializeComponent, deserializeComponent);
	project::addSerializer<ecs::CFontAnchor>(registry, "FontAnchor", "rig.font.anchor",
											 serializeAnchor, deserializeAnchor);
	project::addSerializer<ecs::CFontKern>(registry, "FontKern", "rig.font.kern", serializeKern,
										   deserializeKern);
	project::addSerializer<ecs::CFontGroup>(registry, "FontGroup", "rig.font.group", serializeGroup,
											deserializeGroup);
	project::addSerializer<ecs::CFontAxis>(registry, "FontAxis", "rig.font.axis", serializeAxis,
										   deserializeAxis);
	project::addSerializer<ecs::CFontAvar>(registry, "FontAvar", "rig.font.avar", serializeAvar,
										   deserializeAvar);
	project::addSerializer<ecs::CFontCell>(registry, "FontCell", "rig.font.cell", serializeCell,
										   deserializeCell);
}

} // namespace font
} // namespace rigkit
