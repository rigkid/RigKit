#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigFontComponent/src/rigFontComponent.h"
#include "packs/rigProject/src/rigProject.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CName.h"
#include "FontFind.h"
#include "FontShape.h"
#include "rigFont.h"

#include <spdlog/spdlog.h>

ShapeApp::ShapeApp() {
	window().width = 640;
	window().height = 320;
	window().title = "rigFont - shape";
	settings().appName = "shape";
}

void ShapeApp::setup() {
	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack(std::make_shared<rigkit::rigComponent>());
	packs->registerPack(std::make_shared<rigkit::rigProject>());
	packs->registerPack(std::make_shared<rigkit::rigFontComponent>());
	packs->registerPack(std::make_shared<rigkit::rigFont>());
	packs->initAll();
	packs->setupAll();

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	auto face = ecs->createEntity("LiveFace Regular");
	rigkit::ecs::CFontFace facePod;
	facePod.family = "LiveFace";
	ecs->addComponent(face, facePod);

	auto a = ecs->createEntity("A");
	rigkit::ecs::CFontGlyph ga;
	ga.unicodes = {65};
	ga.width = 600.f;
	ecs->addComponent(a, ga);
	rigkit::ecs::CName na;
	na.stableId = "A";
	na.name = "A";
	ecs->addComponent(a, na);

	auto v = ecs->createEntity("V");
	rigkit::ecs::CFontGlyph gv;
	gv.unicodes = {86};
	gv.width = 600.f;
	ecs->addComponent(v, gv);
	rigkit::ecs::CName nv;
	nv.stableId = "V";
	nv.name = "V";
	ecs->addComponent(v, nv);

	rigkit::font::setKernPair(*ecs, "A", "V", -40.f);
	const auto run = rigkit::font::shapeString(*ecs, "AV", true);
	spdlog::info("shape AV width {}", run.width);
}
