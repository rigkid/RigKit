#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigShowComponent/src/rig/show.h"
#include "packs/rigShowComponent/src/rigShowComponent.h"

#include <spdlog/spdlog.h>

void ShowDeckApp::setup() {
	spdlog::info("showDeck - CVisualizer playlist");
	m_engine->setClearColor(0.08f, 0.09f, 0.12f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigProject>();
	packs->registerPack<rigkit::rigShowComponent>();
	packs->initAll();
	packs->setupAll();

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	auto deck = rig::makeVisualizer(*ecs, "data/presets", "deck");
	rig::addVisualizerPreset(*ecs, deck, "data/presets/warm.vis", "warm");
	spdlog::info("showDeck - deck entity ready");
}
