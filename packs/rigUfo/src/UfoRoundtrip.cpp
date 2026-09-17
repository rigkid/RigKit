#include "UfoRoundtrip.h"

#include "CCode.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CFontKern.h"
#include "CPath.h"
#include "ContractImport.h"
#include "FontFind.h"
#include "UfoIo.h"

#include <cmath>

namespace rigkit {
namespace ufo {
namespace {

bool nearEq(float a, float b) {
	return std::fabs(a - b) <= 0.5f;
}

bool samePath(const ecs::CPath& a, const ecs::CPath& b) {
	if (a.commands.size() != b.commands.size()) {
		return false;
	}
	for (size_t i = 0; i < a.commands.size(); ++i) {
		if (a.commands[i].type != b.commands[i].type) {
			return false;
		}
		if (!nearEq(a.commands[i].p.x, b.commands[i].p.x) ||
			!nearEq(a.commands[i].p.y, b.commands[i].p.y)) {
			return false;
		}
	}
	return true;
}

} // namespace

bool roundtripFromContractJson(MEcs& ecs, const project::ComponentSerializerRegistry& codecs,
							   const std::string& jsonPath, const std::string& ufoDir,
							   std::string* error) {
	auto imported = project::importContractFile(ecs, jsonPath, codecs);
	if (!imported.ok) {
		if (error) {
			*error = imported.error.empty() ? "import failed" : imported.error;
		}
		return false;
	}
	font::rewriteImportedRefs(ecs, imported.entities);

	font::setKernPair(ecs, "A", "V", -55.f);
	std::string feaMark = "# ufomat-smoke\n";
	for (auto e : ecs.registry().view<ecs::CCode>()) {
		auto& code = ecs.registry().get<ecs::CCode>(e);
		if (code.language == "fea") {
			if (code.text.find(feaMark) == std::string::npos) {
				code.text += feaMark;
			}
			break;
		}
	}

	ecs::CPath pathA;
	float widthA = 0.f;
	bool haveA = false;
	if (auto a = font::findNamed(ecs, "A"); a != entt::null) {
		haveA = ecs.hasComponent<ecs::CFontGlyph>(a);
		if (haveA) {
			widthA = ecs.getComponent<ecs::CFontGlyph>(a).width;
		}
		if (ecs.hasComponent<ecs::CPath>(a)) {
			pathA = ecs.getComponent<ecs::CPath>(a);
		}
	}

	std::string saveErr;
	if (!saveUfo(ecs, ufoDir, &saveErr)) {
		if (error) {
			*error = saveErr.empty() ? "save ufo failed" : saveErr;
		}
		return false;
	}

	auto loaded = loadUfo(ecs, ufoDir);
	if (!loaded.ok) {
		if (error) {
			*error = loaded.error.empty() ? "load ufo failed" : loaded.error;
		}
		return false;
	}

	if (ecs.registry().view<ecs::CFontFace>().begin() ==
		ecs.registry().view<ecs::CFontFace>().end()) {
		if (error) {
			*error = "no face after load";
		}
		return false;
	}
	auto a = font::findNamed(ecs, "A");
	if (a == entt::null || !ecs.hasComponent<ecs::CFontGlyph>(a)) {
		if (error) {
			*error = "glyph A missing after load";
		}
		return false;
	}
	if (haveA && !nearEq(ecs.getComponent<ecs::CFontGlyph>(a).width, widthA)) {
		if (error) {
			*error = "glyph A width changed";
		}
		return false;
	}
	if (!pathA.empty()) {
		if (!ecs.hasComponent<ecs::CPath>(a) || !samePath(pathA, ecs.getComponent<ecs::CPath>(a))) {
			if (error) {
				*error = "glyph A path changed";
			}
			return false;
		}
	}
	if (!nearEq(font::kernValue(ecs, "A", "V"), -55.f)) {
		if (error) {
			*error = "written kern AV missing";
		}
		return false;
	}
	bool feaOk = false;
	for (auto e : ecs.registry().view<ecs::CCode>()) {
		const auto& code = ecs.registry().get<ecs::CCode>(e);
		if (code.language == "fea" && code.text.find(feaMark) != std::string::npos) {
			feaOk = true;
		}
	}
	if (!feaOk) {
		if (error) {
			*error = "features.fea mark missing";
		}
		return false;
	}
	return true;
}

} // namespace ufo
} // namespace rigkit
