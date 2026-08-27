#include "app.h"

#include "core/pack/MPack.h"
#include "core/RigKitEngine.h"
#include "packs/rigComponent/src/CRelationship.h"
#include "packs/rigComponent/src/CSelection.h"
#include "packs/rigComponent/src/rig.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigSystems/src/rigSystems.h"

#include <spdlog/spdlog.h>

void MinimalApp::setup() {
	spdlog::info("minimal setup - creators + meshes");
	m_engine->setClearColor(0.12f, 0.14f, 0.18f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->initAll();
	packs->setupAll();

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}

	// Data path: POD shapes/meshes - host Draw + rigSystems present to the window.
	auto rect =
		rig::makeRect(*ecs, 64.f, 64.f, 180.f, 110.f, rig::fill(0.25f, 0.65f, 1.0f), "demo-rect");
	rigkit::ecs::CSelection sel;
	sel.isSelected = true;
	ecs->addComponent<rigkit::ecs::CSelection>(rect, sel);

	rig::makeCircle(*ecs, 360.f, 120.f, 48.f,
					rig::fillAndStroke(1.f, 0.45f, 0.3f, 1.f, 1.f, 1.f, 2.f), "demo-circle");

	// Hierarchy: child local offset under parent - SHierarchy writes world for Draw.
	auto parent = rig::makeRect(*ecs, 200.f, 320.f, 120.f, 80.f, rig::fill(0.55f, 0.35f, 0.9f),
								"demo-parent");
	auto child =
		rig::makeRect(*ecs, 40.f, 30.f, 70.f, 50.f, rig::fill(0.95f, 0.55f, 0.2f), "demo-child");
	rigkit::ecs::CRelationship rel;
	rel.parent = parent;
	ecs->addComponent<rigkit::ecs::CRelationship>(child, rel);

	// Meshes are first-class data too.
	rig::makeMeshTriangle(*ecs, {520.f, 60.f, 0.f}, {640.f, 200.f, 0.f}, {400.f, 200.f, 0.f},
						  rig::fill(0.95f, 0.8f, 0.2f), "demo-tri");
	rig::makeMeshQuad(*ecs, 700.f, 80.f, 140.f, 140.f, rig::fill(0.35f, 0.85f, 0.55f), "demo-quad");
}
