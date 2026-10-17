#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigMusicComponent/src/CMusicClock.h"
#include "packs/rigMusicComponent/src/CMusicTransport.h"
#include "packs/rigMusicComponent/src/rigMusicComponent.h"
#include "packs/rigProject/src/rigProject.h"

#include <spdlog/spdlog.h>

void ClockApp::setup() {
	spdlog::info("clock - CMusicClock / CMusicTransport");
	m_engine->setClearColor(0.08f, 0.09f, 0.12f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigProject>();
	packs->registerPack<rigkit::rigMusicComponent>();
	packs->initAll();
	packs->setupAll();

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}

	auto transport = ecs->createEntity("transport");
	rigkit::ecs::CMusicTransport tr;
	tr.bpm = 120.f;
	tr.playing = true;
	ecs->addComponent(transport, tr);

	auto clock = ecs->createEntity("clock");
	rigkit::ecs::CMusicClock ck;
	ck.ticksPerQuarter = 96;
	ecs->addComponent(clock, ck);
	spdlog::info("clock - 120 bpm transport ready");
}
