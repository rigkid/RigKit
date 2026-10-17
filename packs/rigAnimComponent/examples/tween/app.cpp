#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigAnimComponent/src/CTween.h"
#include "packs/rigAnimComponent/src/rigAnimComponent.h"
#include "packs/rigComponent/src/rig.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigSystems/src/rigSystems.h"

#include <spdlog/spdlog.h>

void TweenApp::setup() {
	spdlog::info("tween - CTween slides a rect");
	m_engine->setClearColor(0.08f, 0.09f, 0.12f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigProject>();
	packs->registerPack<rigkit::rigAnimComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->initAll();
	packs->setupAll();

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}

	rig::makeRect(*ecs, 80.f, 80.f, 160.f, 90.f, rig::fill(0.25f, 0.65f, 1.0f), "block");

	rigkit::ecs::CTween tween;
	tween.target = "block";
	tween.propertyKey = "position.x";
	tween.from = 80.f;
	tween.to = 280.f;
	tween.duration = 2.f;
	tween.easing = "ease-in-out";
	tween.loop = true;
	ecs->addComponent(ecs->createEntity("slide"), tween);
}
