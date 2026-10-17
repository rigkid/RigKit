#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/IMui.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "packs/rigColorspace/src/rigColorspace.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigPixelPlotComponent/src/rigPixelPlotComponent.h"
#include "packs/rigPixelPlotter/src/rigPixelPlotter.h"
#include "packs/rigPixelPlotterUi/src/rigPixelPlotterUi.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigPsd/src/rigPsd.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "CPixelBrush.h"
#include "CPixelLayer.h"
#include "PixelDoc.h"

#include <filesystem>
#include <spdlog/spdlog.h>

PixelEditorApp::PixelEditorApp() {
	window().width = 1280;
	window().height = 800;
	window().title = "example-pixelEditor";
}

void PixelEditorApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
}

void PixelEditorApp::seedDocument() {
	if (!m_plotter) {
		return;
	}
	auto& doc = m_plotter->doc();
	doc.ensureScene();
	doc.canvas().title = "pixelEditor";
	doc.canvas().width = 320;
	doc.canvas().height = 240;
	doc.ensureDefaultBrushes();
	const auto paint = doc.addOverlayLayer("Paint");
	rigkit::ecs::CPixelBrush brush = doc.brushes().current();
	brush.size = 28.f;
	brush.r = 0.85f;
	brush.g = 0.2f;
	brush.b = 0.15f;
	doc.paintStroke(paint, 40.f, 40.f, 200.f, 160.f, brush);
	doc.composite().enabled = true;
	doc.canvas().needsRender = true;
	doc.render();
}

void PixelEditorApp::setup() {
	spdlog::info("example-pixelEditor - pixel layers, paint, PSD");
	m_engine->setClearColor(0.08f, 0.08f, 0.09f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->registerPack<rigkit::rigProject>();
	packs->registerPack<rigkit::rigColorspace>();
	packs->registerPack<rigkit::rigPsd>();
	packs->registerPack<rigkit::rigPixelPlotComponent>();
	packs->registerPack<rigkit::rigPixelPlotter>();
	packs->registerPack<rigkit::rigImGui>();
	packs->registerPack<rigkit::rigPixelPlotterUi>();
	packs->initAll();
	packs->setupAll();

	m_plotter = packs->getPack<rigkit::rigPixelPlotter>();
	m_document = packs->getPack<rigkit::rigProject>();
	if (!m_plotter || !m_document) {
		spdlog::error("example-pixelEditor - missing rigPixelPlotter or rigProject");
		return;
	}

	if (auto* ui = m_engine->getUiManager()) {
		ui->loadWorkspace("Pixel", false);
	}

	if (auto uiPack = packs->getPack<rigkit::rigPixelPlotterUi>()) {
		uiPack->resyncFromDoc();
	}

	if (m_smoke) {
		seedDocument();
		m_smokeOk = runSmoke();
		if (!m_smokeOk) {
			spdlog::error("example-pixelEditor --smoke failed");
		} else {
			spdlog::info("example-pixelEditor --smoke ok");
		}
		if (auto* win = m_engine->getWindow()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
		return;
	}

	seedDocument();
	if (auto uiPack = packs->getPack<rigkit::rigPixelPlotterUi>()) {
		uiPack->resyncFromDoc();
	}
}

bool PixelEditorApp::runSmoke() {
	if (!m_plotter || !m_document) {
		return false;
	}
	auto& doc = m_plotter->doc();
	if (doc.layerEntities().size() < 2) {
		spdlog::error("pixelEditor smoke: expected Plot + Paint layers");
		return false;
	}
	const std::string exportDir = AppPaths::joinPath(AppPaths::getDataDir(), "export");
	std::filesystem::create_directories(exportDir);
	const std::string png = AppPaths::joinPath(exportDir, "smoke.png");
	if (!doc.exportPng(png) || !std::filesystem::exists(png)) {
		spdlog::error("pixelEditor smoke: PNG export failed");
		return false;
	}
	const std::string rigPath = AppPaths::joinPath(exportDir, "smoke.rig");
	doc.ensureProjectEnvelope(rigPath);
	if (!doc.saveProject(*m_document, rigPath) || !doc.loadProject(*m_document, rigPath)) {
		spdlog::error("pixelEditor smoke: .rig round-trip failed");
		return false;
	}
	const std::string psd = AppPaths::joinPath(exportDir, "smoke.psd");
	if (!doc.savePsd(psd) || !std::filesystem::exists(psd)) {
		spdlog::error("pixelEditor smoke: PSD write failed");
		return false;
	}
	if (!doc.loadPsd(psd)) {
		spdlog::error("pixelEditor smoke: PSD read failed");
		return false;
	}
	bool foundPaint = false;
	for (auto e : doc.layerEntities()) {
		if (doc.ecs().hasComponent<rigkit::ecs::CPixelLayer>(e) &&
			doc.ecs().getComponent<rigkit::ecs::CPixelLayer>(e).name == "Paint") {
			foundPaint = true;
		}
	}
	if (!foundPaint) {
		spdlog::error("pixelEditor smoke: Paint layer missing after PSD load");
		return false;
	}
	return true;
}
