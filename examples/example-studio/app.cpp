#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/IMui.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/CommandLineArgs.h"
#include "packs/rigArtboards/src/rigArtboards.h"
#include "packs/rigColorspace/src/rigColorspace.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigCompositor/src/rigCompositor.h"
#include "packs/rigDxf/src/rigDxf.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigImage/src/rigImage.h"
#include "packs/rigPixelPlotComponent/src/rigPixelPlotComponent.h"
#include "packs/rigPixelPlotter/src/rigPixelPlotter.h"
#include "packs/rigPixelPlotterUi/src/rigPixelPlotterUi.h"
#include "packs/rigPlotComponent/src/rigPlotComponent.h"
#include "packs/rigPlotter/src/rigPlotter.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigPsd/src/rigPsd.h"
#include "packs/rigSvg/src/rigSvg.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "packs/rigVectorEditor/src/rigVectorEditor.h"
#include "packs/rigVectorUi/src/rigVectorUi.h"
#include "CPath.h"
#include "ParsedPathDoc.h"
#include "PixelDoc.h"
#include "PlotDoc.h"

#include <spdlog/spdlog.h>

void StudioApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
}

void StudioApp::seedVector() {
	if (!m_plotter) {
		return;
	}
	using Cmd = rigkit::ecs::CPath::Cmd;
	rigkit::plot::ParsedPathDoc parsed;
	parsed.contentW = 200.f;
	parsed.contentH = 160.f;
	rigkit::plot::PathSourceLayer letter;
	letter.name = "Letter";
	rigkit::ecs::CPath path;
	path.commands.push_back({Cmd::MoveTo, {40.f, 40.f}, {}, {}});
	path.commands.push_back({Cmd::LineTo, {160.f, 40.f}, {}, {}});
	path.commands.push_back({Cmd::LineTo, {160.f, 120.f}, {}, {}});
	path.commands.push_back({Cmd::LineTo, {40.f, 120.f}, {}, {}});
	path.commands.push_back({Cmd::Close, {}, {}, {}});
	letter.paths.paths.push_back(std::move(path));
	letter.paths.pathFilled.push_back(1);
	letter.paths.pathColors.push_back({0.2f, 0.45f, 0.9f, 1.f});
	parsed.layers.push_back(std::move(letter));
	m_plotter->doc().commitParsed(parsed, true);
}

void StudioApp::seedPixel() {
	if (!m_pixel) {
		return;
	}
	auto& doc = m_pixel->doc();
	doc.ensureScene();
	doc.canvas().width = 256;
	doc.canvas().height = 192;
	doc.ensureDefaultBrushes();
	const auto paint = doc.addOverlayLayer("Paint");
	auto brush = doc.brushes().current();
	brush.size = 20.f;
	doc.paintDab(paint, 80.f, 80.f, brush);
	doc.composite().enabled = true;
	doc.canvas().needsRender = true;
	doc.render();
}

void StudioApp::setup() {
	spdlog::info("example-studio - Vector / Pixel workspaces");
	m_engine->setClearColor(0.07f, 0.07f, 0.08f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->registerPack<rigkit::rigProject>();
	packs->registerPack<rigkit::rigPlotComponent>();
	packs->registerPack<rigkit::rigColorspace>();
	packs->registerPack<rigkit::rigCompositor>();
	packs->registerPack<rigkit::rigSvg>();
	packs->registerPack<rigkit::rigDxf>();
	packs->registerPack<rigkit::rigVectorEditor>();
	packs->registerPack<rigkit::rigArtboards>();
	packs->registerPack<rigkit::rigImage>();
	packs->registerPack<rigkit::rigPlotter>();
	packs->registerPack<rigkit::rigPsd>();
	packs->registerPack<rigkit::rigPixelPlotComponent>();
	packs->registerPack<rigkit::rigPixelPlotter>();
	packs->registerPack<rigkit::rigImGui>();
	packs->registerPack<rigkit::rigVectorUi>();
	packs->registerPack<rigkit::rigPixelPlotterUi>();
	packs->initAll();
	packs->setupAll();

	m_document = packs->getPack<rigkit::rigProject>();
	m_plotter = packs->getPack<rigkit::rigPlotter>();
	m_pixel = packs->getPack<rigkit::rigPixelPlotter>();

	if (auto* ui = m_engine->getUiManager()) {
		if (!ui->loadWorkspace("Vector", false)) {
			spdlog::warn("example-studio - Vector workspace missing; using session layout");
		}
	}

	seedVector();
	seedPixel();
	if (auto uiPack = packs->getPack<rigkit::rigPixelPlotterUi>()) {
		uiPack->resyncFromDoc();
	}

	if (m_smoke) {
		m_smokeOk = runSmoke();
		if (!m_smokeOk) {
			spdlog::error("example-studio --smoke failed");
		} else {
			spdlog::info("example-studio --smoke ok");
		}
		if (auto* win = m_engine->getWindow()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
	}
}

bool StudioApp::runSmoke() {
	if (!m_plotter || !m_pixel) {
		return false;
	}
	if (m_plotter->doc().layerEntities().empty()) {
		spdlog::error("studio smoke: vector layers missing");
		return false;
	}
	if (m_pixel->doc().layerEntities().size() < 2) {
		spdlog::error("studio smoke: pixel layers missing");
		return false;
	}
	auto* ui = m_engine->getUiManager();
	if (ui) {
		if (!ui->loadWorkspace("Pixel", false) || ui->currentWorkspace() != "Pixel") {
			spdlog::error("studio smoke: Pixel workspace failed");
			return false;
		}
		if (!ui->loadWorkspace("Vector", false) || ui->currentWorkspace() != "Vector") {
			spdlog::error("studio smoke: Vector workspace failed");
			return false;
		}
	}
	return true;
}
