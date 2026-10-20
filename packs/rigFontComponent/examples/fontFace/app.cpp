#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "ecs/MEcs.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/PropertiesWindow.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigProject/src/rigProject.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CFontLayer.h"
#include "CName.h"
#include "CRelationship.h"
#include "CSelection.h"
#include "rigFontComponent.h"

#include <spdlog/spdlog.h>

FontFaceApp::FontFaceApp() {
	window().width = 1200;
	window().height = 760;
	window().title = "rigFontComponent - fontFace";
	settings().appName = "fontFace";
}

void FontFaceApp::setup() {
	spdlog::info("fontFace - seed CFontFace, a default layer, and glyphs");
	m_engine->setClearColor(0.12f, 0.14f, 0.18f, 1.f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigProject>();
	packs->registerPack<rigkit::rigFontComponent>();
	packs->registerPack<rigkit::rigImGui>();
	packs->initAll();
	packs->setupAll();

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}

	auto face = ecs->createEntity("LiveFace Regular");
	rigkit::ecs::CFontFace facePod;
	facePod.family = "LiveFace";
	facePod.style = "Regular";
	facePod.unitsPerEm = 1000.f;
	facePod.ascender = 800.f;
	facePod.descender = -200.f;
	facePod.capHeight = 700.f;
	facePod.xHeight = 500.f;
	ecs->addComponent(face, facePod);

	auto layer = ecs->createEntity("public.default");
	ecs->addComponent(layer, rigkit::ecs::CFontLayer{});
	rigkit::ecs::CName layerName;
	layerName.name = "public.default";
	layerName.stableId = "public.default";
	ecs->addComponent(layer, layerName);
	rigkit::ecs::CRelationship layerRel;
	layerRel.parent = face;
	ecs->addComponent(layer, layerRel);

	auto addGlyph = [&](const char* name, uint32_t cp, float width) {
		auto glyph = ecs->createEntity(name);
		rigkit::ecs::CFontGlyph g;
		g.unicodes = {cp};
		g.width = width;
		ecs->addComponent(glyph, g);
		rigkit::ecs::CName named;
		named.name = name;
		named.stableId = name;
		ecs->addComponent(glyph, named);
		rigkit::ecs::CRelationship rel;
		rel.parent = layer;
		ecs->addComponent(glyph, rel);
	};
	addGlyph("space", 32, 500.f);
	addGlyph("A", 65, 600.f);
	addGlyph("B", 66, 620.f);

	rigkit::ecs::CSelection sel;
	sel.isSelected = true;
	sel.selectionIndex = 0;
	ecs->addComponent(face, sel);

	if (auto* mui = dynamic_cast<rigkit::Mui*>(m_engine->getUiManager())) {
		mui->addHostPanel(rigkit::HostPanel::Scene);
		mui->addHostPanel(rigkit::HostPanel::Properties);
		mui->setFirstRunHostDockLayout();
		if (auto* wm = mui->getWindowManager()) {
			if (auto props = wm->getWindow<rigkit::PropertiesWindow>("Properties")) {
				props->setSelectedEntity(entt::to_integral(face));
			}
		}
	}
}
