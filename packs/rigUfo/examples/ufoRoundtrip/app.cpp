#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigFontComponent/src/rigFontComponent.h"
#include "packs/rigProject/src/rigProject.h"
#include "Glif.h"
#include "Plist.h"
#include "rigUfo.h"

#include <spdlog/spdlog.h>

UfoRoundtripApp::UfoRoundtripApp() {
	window().width = 480;
	window().height = 240;
	window().title = "rigUfo - ufoRoundtrip";
	settings().appName = "ufoRoundtrip";
}

void UfoRoundtripApp::setup() {
	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack(std::make_shared<rigkit::rigComponent>());
	packs->registerPack(std::make_shared<rigkit::rigProject>());
	packs->registerPack(std::make_shared<rigkit::rigFontComponent>());
	packs->registerPack(std::make_shared<rigkit::rigUfo>());
	packs->initAll();
	packs->setupAll();

	rigkit::ufo::Plist info;
	info.kind = rigkit::ufo::Plist::Dict;
	info.dict["familyName"] = rigkit::ufo::Plist::str("LiveFace");
	auto xml = rigkit::ufo::writePlist(info);
	rigkit::ufo::Plist back;
	const bool ok = rigkit::ufo::parsePlist(xml, back);
	spdlog::info("ufoRoundtrip plist {}", ok ? "ok" : "fail");
}
