#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigCadComponent/src/rig/cad.h"
#include "packs/rigCadComponent/src/rigCadComponent.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigProject/src/rigProject.h"

#include <spdlog/spdlog.h>

void CadBoxApp::setup() {
	spdlog::info("cadBox - CCadBox stock");
	m_engine->setClearColor(0.08f, 0.09f, 0.12f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigProject>();
	packs->registerPack<rigkit::rigCadComponent>();
	packs->initAll();
	packs->setupAll();

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	rig::makeCadBox(*ecs, 2.f, 1.5f, 1.2f, true, "stock");
	spdlog::info("cadBox - stock entity ready");
}
