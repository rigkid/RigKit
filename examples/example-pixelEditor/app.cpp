#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/IMui.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "packs/rigColorspace/src/ColorProfiles.h"
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
#include "PixelPlotterPanel.h"

#include <filesystem>
#include <spdlog/spdlog.h>

void PixelEditorApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
}

void PixelEditorApp::seedSmokeDocument() {
	if (!m_plotter) {
		return;
	}
	auto& doc = m_plotter->doc();
	doc.ensureScene();
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
	m_uiPack = packs->getPack<rigkit::rigPixelPlotterUi>();
	m_document = packs->getPack<rigkit::rigProject>();
	if (!m_plotter || !m_document) {
		spdlog::error("example-pixelEditor - missing rigPixelPlotter or rigProject");
		return;
	}

	m_mui = m_engine ? rigkit::authorMui(*m_engine) : nullptr;
	if (m_mui) {
		m_mui->loadWorkspace("Pixel", false);
	}

	if (auto uiPack = packs->getPack<rigkit::rigPixelPlotterUi>()) {
		uiPack->resyncFromDoc();
	}

	if (m_smoke) {
		seedSmokeDocument();
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

	// Interactive start has no document window. File New / Open / Open Recent
	// creates the first world. Demo content is a smoke fixture only.
}

void PixelEditorApp::onUiAttached() {
	m_mui = m_engine ? rigkit::authorMui(*m_engine) : nullptr;
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
	{
		entt::entity paint = entt::null;
		for (auto e : doc.layerEntities()) {
			if (doc.ecs().hasComponent<rigkit::ecs::CPixelLayer>(e) &&
				doc.ecs().getComponent<rigkit::ecs::CPixelLayer>(e).name == "Paint") {
				paint = e;
			}
		}
		auto brush = doc.brushes().current();
		if (paint == entt::null || !doc.paintDab(paint, 80.f, 60.f, brush) || !doc.recomposite()) {
			spdlog::error("pixelEditor smoke: GPU paint/recomposite failed");
			return false;
		}
		doc.syncPaintLayer(paint);
		rigkit::PixelDoc::RasterPatch undoBefore;
		rigkit::PixelDoc::RasterPatch undoAfter;
		doc.beginPaintCapture(paint);
		const bool dabOk = doc.paintDab(paint, 290.f, 30.f, brush);
		const bool capOk = doc.endPaintCapture(undoBefore, undoAfter);
		if (!dabOk || !capOk || undoBefore.empty() || undoBefore.rgba == undoAfter.rgba) {
			spdlog::error("pixelEditor smoke: raster undo capture failed dab={} cap={} empty={} "
						  "same={} rect={}x{}",
						  dabOk, capOk, undoBefore.empty(), undoBefore.rgba == undoAfter.rgba,
						  undoBefore.w, undoBefore.h);
			return false;
		}
		doc.applyRasterPatch(undoBefore);
		const auto& raster = doc.ecs().getComponent<rigkit::ecs::CPixelLayerRaster>(paint);
		const uint8_t* px =
			raster.rgba.data() + (static_cast<size_t>(undoBefore.y) * raster.w + undoBefore.x) * 4;
		if (px[0] != undoBefore.rgba[0] || px[3] != undoBefore.rgba[3] || !doc.render()) {
			spdlog::error("pixelEditor smoke: raster undo apply failed");
			return false;
		}
		{
			// Eraser: paint then erase the same spot; alpha must go back to 0.
			auto& raster = doc.ecs().getComponent<rigkit::ecs::CPixelLayerRaster>(paint);
			auto alphaAt = [&raster](int x, int y) {
				return raster.rgba[(static_cast<size_t>(y) * raster.w + x) * 4 + 3];
			};
			doc.paintDab(paint, 100.f, 200.f, brush);
			doc.syncPaintLayer(paint);
			const int painted = alphaAt(100, 200);
			doc.paintDab(paint, 100.f, 200.f, brush, true);
			doc.syncPaintLayer(paint);
			if (painted == 0 || alphaAt(100, 200) != 0) {
				spdlog::error("pixelEditor smoke: eraser failed");
				return false;
			}
			// Tip: a real PNG tip paints; a missing tip falls back to round.
			const std::string tipDir = AppPaths::joinPath(AppPaths::getDataDir(), "export");
			std::filesystem::create_directories(tipDir);
			const std::string tipPath = AppPaths::joinPath(tipDir, "tip.png");
			auto tipBrush = brush;
			tipBrush.tipPath = tipPath;
			if (!doc.exportPng(tipPath) || !doc.paintDab(paint, 250.f, 200.f, tipBrush)) {
				spdlog::error("pixelEditor smoke: tip dab failed");
				return false;
			}
			doc.syncPaintLayer(paint);
			if (alphaAt(250, 200) == 0) {
				spdlog::error("pixelEditor smoke: tip dab painted nothing");
				return false;
			}
			tipBrush.tipPath = "does-not-exist.png";
			if (!doc.paintDab(paint, 200.f, 200.f, tipBrush)) {
				spdlog::error("pixelEditor smoke: missing tip must fall back to round");
				return false;
			}
			doc.syncPaintLayer(paint);
		}
		auto cropBefore = doc.captureCropState();
		if (!doc.cropCanvas(10, 10, 160, 120) || doc.canvas().width != 160 ||
			doc.canvas().height != 120 || !doc.render()) {
			spdlog::error("pixelEditor smoke: crop failed");
			return false;
		}
		doc.restoreCropState(cropBefore);
		if (doc.canvas().width != 320 || doc.canvas().height != 240 || !doc.render() ||
			!doc.cropCanvas(10, 10, 160, 120) || !doc.render()) {
			spdlog::error("pixelEditor smoke: crop undo restore failed");
			return false;
		}
		if (!doc.scaleCanvas(80, 60) || doc.canvas().width != 80 || doc.canvas().height != 60 ||
			!doc.render()) {
			spdlog::error("pixelEditor smoke: image scale failed");
			return false;
		}
		{
			const auto& scaled = doc.ecs().getComponent<rigkit::ecs::CPixelLayerRaster>(paint);
			if (scaled.w != 80 || scaled.h != 60) {
				spdlog::error("pixelEditor smoke: image scale left layer at {}x{}", scaled.w,
							  scaled.h);
				return false;
			}
		}
		if (!doc.scaleCanvas(160, 120) || !doc.render()) {
			spdlog::error("pixelEditor smoke: image scale back failed");
			return false;
		}
		{
			// Undo snapshot round-trip (not multi-doc - that uses exchangeDocument).
			doc.paintDab(paint, 80.f, 60.f, brush);
			doc.syncPaintLayer(paint);
			rigkit::PixelDoc::ProjectState snap;
			doc.captureProject(snap);
			if (!doc.restoreProject(snap)) {
				spdlog::error("pixelEditor smoke: project restore failed");
				return false;
			}
		}
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
	// File > New / Open create parallel EcsDocument worlds via the UI.
	if (!m_uiPack || !m_uiPack->ui()) {
		spdlog::error("pixelEditor smoke: no PixelPlotterUi for New/Open");
		return false;
	}
	auto* ui = m_uiPack->ui();
	// No starter Untitled - File New creates session 0, Open adds session 1.
	spdlog::info("pixelEditor smoke: File New...");
	if (!ui->newDocument()) {
		spdlog::error("pixelEditor smoke: File New failed");
		return false;
	}
	{
		auto& live = m_plotter->doc();
		if (live.canvas().title != "Untitled" || live.canvas().width != 640 ||
			live.canvas().height != 480 || !live.projectPath().empty() || !live.render()) {
			spdlog::error(
				"pixelEditor smoke: File New not a factory blank title='{}' {}x{} path='{}'",
				live.canvas().title, live.canvas().width, live.canvas().height, live.projectPath());
			return false;
		}
	}
	spdlog::info("pixelEditor smoke: File Open image...");
	if (!ui->openImageAsDocument(png)) {
		spdlog::error("pixelEditor smoke: File Open image failed");
		return false;
	}
	{
		auto& live = m_plotter->doc();
		if (live.canvas().title != "smoke" || !live.render()) {
			spdlog::error("pixelEditor smoke: File Open image bad title='{}'", live.canvas().title);
			return false;
		}
	}
	constexpr int kNewSession = 0;
	constexpr int kImageSession = 1;
	spdlog::info("pixelEditor smoke: switch back to New session...");
	if (!ui->switchDocument(kNewSession)) {
		spdlog::error("pixelEditor smoke: switchDocument({}) failed", kNewSession);
		return false;
	}
	{
		auto& live = m_plotter->doc();
		if (live.canvas().title != "Untitled" || live.canvas().width != 640 ||
			live.canvas().height != 480) {
			spdlog::error("pixelEditor smoke: parked New session lost (title='{}' {}x{})",
						  live.canvas().title, live.canvas().width, live.canvas().height);
			return false;
		}
	}
	if (!ui->switchDocument(kImageSession) || m_plotter->doc().canvas().title != "smoke") {
		spdlog::error("pixelEditor smoke: switchDocument({}) failed title='{}'", kImageSession,
					  m_plotter->doc().canvas().title);
		return false;
	}
	spdlog::info("pixelEditor smoke: parallel New/Open/switch ok ({}x{})",
				 m_plotter->doc().canvas().width, m_plotter->doc().canvas().height);

	// Optional pro-print path when myProfiles is deployed next to the exe.
	{
		const std::string profiles =
			AppPaths::joinPath(AppPaths::getDataDir(), "icc/myProfiles");
		rigkit::color::setProfilesRoot(profiles);
		const std::string srgb = rigkit::color::resolveWellKnownProfile("srgb");
		const std::string pso = rigkit::color::resolveWellKnownProfile("pso-coated-v3");
		if (srgb.empty() || pso.empty()) {
			spdlog::info("pixelEditor smoke: skip CMYK ICC path (profiles root empty)");
		} else {
			// Parked New session still has a Paint overlay; image session may not.
			if (!ui->switchDocument(kNewSession)) {
				spdlog::error("pixelEditor smoke: switch to New for CMYK failed");
				return false;
			}
			auto& live = m_plotter->doc();
			live.ensureProjectEnvelope({});
			if (auto* proj = live.projectEnvelope()) {
				proj->outputProfile = pso;
				proj->untaggedRgbProfile = "srgb";
				proj->pdfX = "PDF/X-4";
			}
			if (!live.convertColorMode(rigkit::ecs::PixelColorMode::CMYK) ||
				!live.canvas().isCmyk()) {
				spdlog::error("pixelEditor smoke: RGB to CMYK convert failed");
				return false;
			}
			entt::entity paint = entt::null;
			for (auto e : live.layerEntities()) {
				if (live.ecs().hasComponent<rigkit::ecs::CPixelLayer>(e) &&
					live.ecs().getComponent<rigkit::ecs::CPixelLayer>(e).name == "Paint") {
					paint = e;
				}
			}
			if (paint == entt::null) {
				paint = live.addOverlayLayer("Paint");
			}
			auto brush = live.brushes().current();
			if (paint == entt::null || !live.paintDab(paint, 40.f, 40.f, brush) ||
				!live.render()) {
				spdlog::error("pixelEditor smoke: CMYK paint failed");
				return false;
			}
			const std::string tif = AppPaths::joinPath(exportDir, "smoke_cmyk.tif");
			if (!live.exportImage(tif) || !std::filesystem::exists(tif)) {
				spdlog::error("pixelEditor smoke: CMYK TIFF export failed");
				return false;
			}
			if (!live.convertColorMode(rigkit::ecs::PixelColorMode::RGB)) {
				spdlog::error("pixelEditor smoke: CMYK to RGB convert failed");
				return false;
			}
			spdlog::info("pixelEditor smoke: CMYK mode + TIFF ok");
		}
	}
	return true;
}
