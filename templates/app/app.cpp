#include "app.h"

#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "core/pack/MPack.h"

void MyApp::setup() {
	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->registerPack<rigkit::rigImGui>();
	packs->initAll();
	packs->setupAll();
}
