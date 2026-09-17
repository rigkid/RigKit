#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "ecs/MEcs.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigImGui/src/IWindow.h"
#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CName.h"
#include "CRelationship.h"
#include "rigFontComponent.h"

#include <spdlog/spdlog.h>

namespace {

class GlyphListPanel : public rigkit::IWindow {
  public:
	GlyphListPanel() : IWindow("Glyphs") {}

	void renderContents() override {
		auto* engine = getEngine();
		auto* ecs = engine ? engine->getECSManager() : nullptr;
		if (!ecs) {
			ImGui::TextUnformatted("No ECS");
			return;
		}
		for (auto e : ecs->registry().view<rigkit::ecs::CFontGlyph>()) {
			const auto& g = ecs->registry().get<rigkit::ecs::CFontGlyph>(e);
			const std::string name = ecs->entityName(e);
			ImGui::Text("%s  width %.0f", name.c_str(), static_cast<double>(g.width));
		}
	}
};

} // namespace

FontFaceApp::FontFaceApp() {
	window().width = 640;
	window().height = 400;
	window().title = "rigFontComponent - fontFace";
	settings().appName = "fontFace";
}

void FontFaceApp::setup() {
	spdlog::info("fontFace - seed CFontFace + glyphs");
	m_engine->setClearColor(0.12f, 0.14f, 0.18f, 1.f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack(std::make_shared<rigkit::rigComponent>());
	packs->registerPack(std::make_shared<rigkit::rigSystems>());
	packs->registerPack(std::make_shared<rigkit::rigProject>());
	packs->registerPack(std::make_shared<rigkit::rigFontComponent>());
	packs->registerPack(std::make_shared<rigkit::rigImGui>());
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
	ecs->addComponent(face, facePod);

	auto glyph = ecs->createEntity("A");
	rigkit::ecs::CFontGlyph g;
	g.unicodes = {65};
	g.width = 600.f;
	ecs->addComponent(glyph, g);
	rigkit::ecs::CName named;
	named.name = "A";
	named.stableId = "A";
	ecs->addComponent(glyph, named);
	rigkit::ecs::CRelationship rel;
	rel.parent = face;
	ecs->addComponent(glyph, rel);

	if (auto* mui = dynamic_cast<rigkit::Mui*>(m_engine->getUiManager())) {
		if (auto* wm = mui->getWindowManager()) {
			wm->createWindow<GlyphListPanel>();
			wm->showWindow("Glyphs");
		}
	}
}
