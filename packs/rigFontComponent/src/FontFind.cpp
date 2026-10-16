#include "FontFind.h"

#include "CFontAnchor.h"
#include "CFontComponent.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CFontGroup.h"
#include "CFontKern.h"
#include "CFontLayer.h"
#include "CName.h"
#include "CRelationship.h"

#include <vector>

namespace rigkit {
namespace font {

entt::entity findNamed(MEcs& ecs, const std::string& id) {
	if (id.empty()) {
		return entt::null;
	}
	if (auto e = ecs.findEntity(id); e != entt::null) {
		return e;
	}
	for (auto e : ecs.getAllEntities()) {
		if (!ecs.registry().valid(e)) {
			continue;
		}
		if (!ecs.hasComponent<ecs::CName>(e)) {
			continue;
		}
		const auto& n = ecs.getComponent<ecs::CName>(e);
		if (n.stableId == id || n.name == id) {
			return e;
		}
	}
	return entt::null;
}

void rewriteImportedRefs(MEcs& ecs, const std::unordered_map<std::string, entt::entity>& docIds) {
	auto nameOf = [&](const std::string& id) {
		auto it = docIds.find(id);
		if (it == docIds.end()) {
			return id;
		}
		const std::string n = ecs.entityName(it->second);
		return n.empty() ? id : n;
	};
	for (auto e : ecs.registry().view<ecs::CFontFace>()) {
		auto& face = ecs.registry().get<ecs::CFontFace>(e);
		if (!face.features.empty()) {
			face.features = nameOf(face.features);
		}
	}
	for (auto e : ecs.registry().view<ecs::CFontComponent>()) {
		auto& comp = ecs.registry().get<ecs::CFontComponent>(e);
		if (!comp.glyph.empty()) {
			comp.glyph = nameOf(comp.glyph);
		}
	}
	ensureNames(ecs);
}

void ensureNames(MEcs& ecs) {
	auto stamp = [&](entt::entity e) {
		const std::string name = ecs.entityName(e);
		if (name.empty()) {
			return;
		}
		if (ecs.hasComponent<ecs::CName>(e)) {
			auto& n = ecs.getComponent<ecs::CName>(e);
			if (n.name.empty()) {
				n.name = name;
			}
			if (n.stableId.empty()) {
				n.stableId = name;
			}
			return;
		}
		ecs::CName n;
		n.name = name;
		n.stableId = name;
		ecs.addComponent<ecs::CName>(e, n);
	};
	for (auto e : ecs.registry().view<ecs::CFontFace>()) {
		stamp(e);
	}
	for (auto e : ecs.registry().view<ecs::CFontLayer>()) {
		stamp(e);
	}
	for (auto e : ecs.registry().view<ecs::CFontGlyph>()) {
		stamp(e);
	}
	for (auto e : ecs.registry().view<ecs::CFontGroup>()) {
		stamp(e);
	}
	for (auto e : ecs.registry().view<ecs::CFontAnchor>()) {
		stamp(e);
	}
}

std::string stableName(MEcs& ecs, entt::entity e) {
	if (e == entt::null || !ecs.registry().valid(e)) {
		return {};
	}
	if (ecs.hasComponent<ecs::CName>(e) && !ecs.getComponent<ecs::CName>(e).stableId.empty()) {
		return ecs.getComponent<ecs::CName>(e).stableId;
	}
	return ecs.entityName(e);
}

namespace {

bool exactKern(MEcs& ecs, const std::string& left, const std::string& right, float& out) {
	for (auto e : ecs.registry().view<ecs::CFontKern>()) {
		const auto& k = ecs.registry().get<ecs::CFontKern>(e);
		if (k.left == left && k.right == right) {
			out = k.value;
			return true;
		}
	}
	return false;
}

bool groupHas(const ecs::CFontGroup& g, const std::string& glyph) {
	for (const auto& m : g.members) {
		if (m == glyph) {
			return true;
		}
	}
	return false;
}

bool isLeftGroup(MEcs& ecs, entt::entity e, const ecs::CFontGroup& g) {
	if (g.side == ecs::CFontGroup::Left) {
		return true;
	}
	if (g.side == ecs::CFontGroup::Right) {
		return false;
	}
	const std::string name = stableName(ecs, e);
	return name.rfind("public.kern1.", 0) == 0;
}

bool isRightGroup(MEcs& ecs, entt::entity e, const ecs::CFontGroup& g) {
	if (g.side == ecs::CFontGroup::Right) {
		return true;
	}
	if (g.side == ecs::CFontGroup::Left) {
		return false;
	}
	const std::string name = stableName(ecs, e);
	return name.rfind("public.kern2.", 0) == 0;
}

} // namespace

float kernValue(MEcs& ecs, const std::string& left, const std::string& right) {
	if (left.empty() || right.empty()) {
		return 0.f;
	}
	float v = 0.f;
	if (exactKern(ecs, left, right, v)) {
		return v;
	}
	std::vector<std::string> leftGroups;
	std::vector<std::string> rightGroups;
	for (auto e : ecs.registry().view<ecs::CFontGroup>()) {
		const auto& g = ecs.registry().get<ecs::CFontGroup>(e);
		const std::string name = stableName(ecs, e);
		if (name.empty()) {
			continue;
		}
		if (isLeftGroup(ecs, e, g) && groupHas(g, left)) {
			leftGroups.push_back(name);
		}
		if (isRightGroup(ecs, e, g) && groupHas(g, right)) {
			rightGroups.push_back(name);
		}
	}
	for (const auto& lg : leftGroups) {
		if (exactKern(ecs, lg, right, v)) {
			return v;
		}
	}
	for (const auto& rg : rightGroups) {
		if (exactKern(ecs, left, rg, v)) {
			return v;
		}
	}
	for (const auto& lg : leftGroups) {
		for (const auto& rg : rightGroups) {
			if (exactKern(ecs, lg, rg, v)) {
				return v;
			}
		}
	}
	return 0.f;
}

entt::entity findKernEntity(MEcs& ecs, const std::string& left, const std::string& right) {
	for (auto e : ecs.registry().view<ecs::CFontKern>()) {
		const auto& k = ecs.registry().get<ecs::CFontKern>(e);
		if (k.left == left && k.right == right) {
			return e;
		}
	}
	return entt::null;
}

void setKernPair(MEcs& ecs, const std::string& left, const std::string& right, float value) {
	if (left.empty() || right.empty()) {
		return;
	}
	auto existing = findKernEntity(ecs, left, right);
	if (value == 0.f) {
		if (existing != entt::null) {
			ecs.destroyEntity(existing);
		}
		return;
	}
	if (existing != entt::null) {
		ecs.getComponent<ecs::CFontKern>(existing).value = value;
		return;
	}
	entt::entity face = entt::null;
	for (auto e : ecs.registry().view<ecs::CFontFace>()) {
		face = e;
		break;
	}
	auto e = ecs.createEntity();
	ecs::CFontKern k;
	k.left = left;
	k.right = right;
	k.value = value;
	ecs.addComponent(e, k);
	if (face != entt::null) {
		ecs::CRelationship rel;
		rel.parent = face;
		ecs.addComponent(e, rel);
	}
}

std::vector<entt::entity> layersOfFace(MEcs& ecs, entt::entity face) {
	std::vector<entt::entity> out;
	for (auto e : ecs.registry().view<ecs::CFontLayer>()) {
		if (face != entt::null && ecs.hasComponent<ecs::CRelationship>(e) &&
			ecs.getComponent<ecs::CRelationship>(e).parent != face) {
			continue;
		}
		out.push_back(e);
	}
	return out;
}

entt::entity findGlyphInLayer(MEcs& ecs, entt::entity layer, const std::string& name) {
	if (layer == entt::null || name.empty()) {
		return entt::null;
	}
	for (auto e : ecs.registry().view<ecs::CFontGlyph>()) {
		if (!ecs.hasComponent<ecs::CRelationship>(e) ||
			ecs.getComponent<ecs::CRelationship>(e).parent != layer) {
			continue;
		}
		if (stableName(ecs, e) == name) {
			return e;
		}
	}
	return entt::null;
}

entt::entity findGlyphByUnicode(MEcs& ecs, uint32_t cp) {
	entt::entity fallback = entt::null;
	for (auto e : ecs.registry().view<ecs::CFontGlyph>()) {
		const auto& g = ecs.registry().get<ecs::CFontGlyph>(e);
		for (uint32_t u : g.unicodes) {
			if (u == cp) {
				return e;
			}
		}
		if (stableName(ecs, e) == ".notdef") {
			fallback = e;
		}
	}
	return fallback;
}

} // namespace font
} // namespace rigkit
