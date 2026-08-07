#include "PackLoader.h"
#include <spdlog/spdlog.h>
#include "MPack.h"
#include "PackRegistry.h"

void rigkit::PackLoader::registerAvailablePacks(MPack* packManager,
												const std::vector<std::string>& packNames) {
	if (!packManager) {
		spdlog::error("[PackLoader] No pack manager provided");
		return;
	}

	std::vector<std::string> namesToRegister = packNames;
	if (namesToRegister.empty()) {
		namesToRegister = getAvailablePackNames();
	}

	for (const auto& name : namesToRegister) {
		if (registerPack(packManager, name)) {
			spdlog::info("[PackLoader] Successfully registered pack: {}", name);
		} else {
			spdlog::warn("[PackLoader] Failed to register pack: {}", name);
		}
	}
}

bool rigkit::PackLoader::registerPack(MPack* packManager, const std::string& packName) {
	if (!packManager) {
		spdlog::error("[PackLoader] No pack manager provided");
		return false;
	}

	// Use the PackRegistry to create the pack
	auto pack = PackRegistry::instance().create(packName);
	if (pack) {
		packManager->registerPack(pack);
		return true;
	} else {
		spdlog::error("[PackLoader] Unknown pack: {}", packName);
		return false;
	}
}

std::vector<std::string> rigkit::PackLoader::getAvailablePackNames() {
	// rigImGui = rigImGui (default UI pack). Not required by SUDE.
	return {"rigImGui",	 "rigBlend2D",	"rigRender3D",		"rigObj",
			"rigAssimp", "rigMeshEdit", "rigNodeComponent", "rigNodeEditor",
			"rigOsc",	 "rigMarkdown", "rigTemplate"};
}