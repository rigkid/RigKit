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
#include "packs/rigProject/src/Rigz.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigPsd/src/rigPsd.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "CAssetRef.h"
#include "CCurve.h"
#include "CPixelAdjustment.h"
#include "CPixelBrush.h"
#include "CPixelLayer.h"
#include "CPixelSource.h"
#include "PixelDoc.h"
#include "PixelPlotterPanel.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

void PixelEditorApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
	if (const auto recipe = args.getValue("recipe")) {
		m_recipePath = *recipe;
		m_batch = true;
	}
	if (const auto inDir = args.getValue("in")) {
		m_batchIn = *inDir;
		m_batch = true;
	}
	if (const auto outDir = args.getValue("out")) {
		m_batchOut = *outDir;
		m_batch = true;
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

	if (m_batch) {
		m_batchOk = runBatch();
		if (!m_batchOk) {
			spdlog::error("example-pixelEditor --recipe batch failed");
		} else {
			spdlog::info("example-pixelEditor --recipe batch ok");
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
		{
			entt::entity plot = entt::null;
			for (auto e : doc.layerEntities()) {
				if (doc.ecs().getComponent<rigkit::ecs::CPixelLayer>(e).kind ==
					rigkit::ecs::PixelLayerKind::Plot) {
					plot = e;
				}
			}
			auto tiny = brush;
			tiny.size = 4.f;
			if (plot == entt::null || doc.nudgeLayerContent(plot, 4, 0) ||
				!doc.paintDab(paint, 16.f, 230.f, tiny)) {
				spdlog::error("pixelEditor smoke: layer nudge setup failed");
				return false;
			}
			doc.syncPaintLayer(paint);
			const auto& nudged = doc.ecs().getComponent<rigkit::ecs::CPixelLayerRaster>(paint);
			auto alphaAt = [&nudged](int x, int y) {
				return nudged.rgba[(static_cast<size_t>(y) * nudged.w + x) * 4 + 3];
			};
			int hx = -1;
			int hy = -1;
			uint8_t ha = 0;
			for (int y = 226; y <= 234; ++y) {
				for (int x = 10; x <= 22; ++x) {
					const uint8_t a = alphaAt(x, y);
					if (a > ha) {
						ha = a;
						hx = x;
						hy = y;
					}
				}
			}
			if (ha == 0 || !doc.nudgeLayerContent(paint, 40, 0) || alphaAt(hx, hy) != 0 ||
				alphaAt(hx + 40, hy) != ha) {
				spdlog::error("pixelEditor smoke: layer nudge failed");
				return false;
			}
		}
		{
			doc.brushes().background = {0.1f, 0.2f, 0.9f, 1.f};
			if (!doc.fillGradient(paint, 0, 0.f, 0.f, 319.f, 0.f, 0.9f, 0.1f, 0.1f, 1.f, 0.1f, 0.2f,
								  0.9f, 1.f)) {
				spdlog::error("pixelEditor smoke: linear gradient fill failed");
				return false;
			}
			doc.syncPaintLayer(paint);
			const auto& gR = doc.ecs().getComponent<rigkit::ecs::CPixelLayerRaster>(paint);
			const uint8_t* left = gR.rgba.data();
			const uint8_t* right = gR.rgba.data() + (319u * 4u);
			if (left[0] < 200 || right[2] < 200) {
				spdlog::error(
					"pixelEditor smoke: gradient ends look wrong L=({},{},{}) R=({},{},{})",
					left[0], left[1], left[2], right[0], right[1], right[2]);
				return false;
			}
			if (!doc.strokeLine(paint, 10.f, 10.f, 100.f, 80.f, 0.f, 1.f, 0.f, 1.f, 4.f) ||
				!doc.strokeRect(paint, 120.f, 20.f, 180.f, 70.f, 1.f, 1.f, 0.f, 1.f, 3.f) ||
				!doc.strokeEllipse(paint, 200.f, 20.f, 280.f, 90.f, 0.f, 1.f, 1.f, 1.f, 3.f)) {
				spdlog::error("pixelEditor smoke: shape strokes failed");
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
	// PSD before .rig: paint rasters live in memory / GPU, not in the project envelope.
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
	{
		// CMYK PSD format round-trip (no ICC): C,M,Y,K bytes + mode 4.
		doc.canvas().colorMode = static_cast<int>(rigkit::ecs::PixelColorMode::CMYK);
		entt::entity cmykPaint = entt::null;
		for (auto e : doc.layerEntities()) {
			if (doc.ecs().hasComponent<rigkit::ecs::CPixelLayer>(e) &&
				doc.ecs().getComponent<rigkit::ecs::CPixelLayer>(e).name == "Paint") {
				cmykPaint = e;
				break;
			}
		}
		if (cmykPaint == entt::null) {
			spdlog::error("pixelEditor smoke: no Paint for CMYK PSD");
			return false;
		}
		auto& cR = doc.ecs().hasComponent<rigkit::ecs::CPixelLayerRaster>(cmykPaint)
					   ? doc.ecs().getComponent<rigkit::ecs::CPixelLayerRaster>(cmykPaint)
					   : doc.ecs().addComponent<rigkit::ecs::CPixelLayerRaster>(cmykPaint, {});
		cR.resize(doc.canvas().width, doc.canvas().height, rigkit::ecs::PixelRasterFormat::CMYK8);
		cR.rgba[0] = 10;
		cR.rgba[1] = 20;
		cR.rgba[2] = 30;
		cR.rgba[3] = 40;
		++doc.ecs().getComponent<rigkit::ecs::CPixelLayer>(cmykPaint).contentVersion;
		const std::string cmykPsd = AppPaths::joinPath(exportDir, "smoke_cmyk.psd");
		if (!doc.savePsd(cmykPsd) || !std::filesystem::exists(cmykPsd)) {
			spdlog::error("pixelEditor smoke: CMYK PSD write failed");
			return false;
		}
		if (!doc.loadPsd(cmykPsd) || !doc.canvas().isCmyk()) {
			spdlog::error("pixelEditor smoke: CMYK PSD read failed mode={}",
						  doc.canvas().colorMode);
			return false;
		}
		bool inkOk = false;
		for (auto e : doc.layerEntities()) {
			if (!doc.ecs().hasComponent<rigkit::ecs::CPixelLayer>(e) ||
				doc.ecs().getComponent<rigkit::ecs::CPixelLayer>(e).name != "Paint" ||
				!doc.ecs().hasComponent<rigkit::ecs::CPixelLayerRaster>(e)) {
				continue;
			}
			const auto& R = doc.ecs().getComponent<rigkit::ecs::CPixelLayerRaster>(e);
			if (!R.isCmyk() || R.empty()) {
				continue;
			}
			inkOk = R.rgba[0] == 10 && R.rgba[1] == 20 && R.rgba[2] == 30 && R.rgba[3] == 40;
			break;
		}
		if (!inkOk) {
			spdlog::error("pixelEditor smoke: CMYK PSD ink round-trip mismatch");
			return false;
		}
		spdlog::info("pixelEditor smoke: CMYK PSD ok");
		doc.canvas().colorMode = static_cast<int>(rigkit::ecs::PixelColorMode::RGB);
	}
	const std::string rigPath = AppPaths::joinPath(exportDir, "smoke.rig");
	doc.ensureProjectEnvelope(rigPath);
	if (!doc.saveProject(*m_document, rigPath) || !doc.loadProject(*m_document, rigPath)) {
		spdlog::error("pixelEditor smoke: .rig round-trip failed");
		return false;
	}
	// File > New / Open create parallel EcsDocument worlds via the UI.
	if (!m_uiPack || !m_uiPack->ui()) {
		spdlog::error("pixelEditor smoke: no PixelPlotterUi for New/Open");
		return false;
	}
	auto* ui = m_uiPack->ui();
	// No starter session. File New adopts the smoke seed and replaces it with a
	// blank; Open parks that and adds the image document.
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

	{
		auto& live = m_plotter->doc();
		const auto curves = live.addCurvesAdjustmentLayer("Curves");
		if (curves == entt::null ||
			!live.ecs().hasComponent<rigkit::ecs::CPixelAdjustment>(curves)) {
			spdlog::error("pixelEditor smoke: Curves adjustment layer failed");
			return false;
		}
		auto& adj = live.ecs().getComponent<rigkit::ecs::CPixelAdjustment>(curves);
		adj.master.points = {{0.f, 0.f}, {0.5f, 0.35f}, {1.f, 1.f}};
		adj.master.preset = rigkit::ecs::CCurve::Preset::Custom;
		rigkit::curve::ensureValid(adj.master);
		live.bumpLayerContent(curves);
		if (!live.render()) {
			spdlog::error("pixelEditor smoke: Curves composite failed");
			return false;
		}
		spdlog::info("pixelEditor smoke: Curves adjustment ok");

		const auto levels = live.addLevelsAdjustmentLayer("Levels");
		if (levels == entt::null ||
			!live.ecs().hasComponent<rigkit::ecs::CPixelAdjustment>(levels)) {
			spdlog::error("pixelEditor smoke: Levels adjustment layer failed");
			return false;
		}
		auto& lev = live.ecs().getComponent<rigkit::ecs::CPixelAdjustment>(levels);
		lev.inBlack = 0.08f;
		lev.inWhite = 0.92f;
		lev.inGamma = 1.1f;
		live.bumpLayerContent(levels);
		if (!live.render()) {
			spdlog::error("pixelEditor smoke: Levels composite failed");
			return false;
		}
		spdlog::info("pixelEditor smoke: Levels adjustment ok");
	}

	// Optional pro-print path when myProfiles is deployed next to the exe.
	{
		const std::string profiles = AppPaths::joinPath(AppPaths::getDataDir(), "icc/myProfiles");
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
			if (paint == entt::null || !live.paintDab(paint, 40.f, 40.f, brush) || !live.render()) {
				spdlog::error("pixelEditor smoke: CMYK paint failed");
				return false;
			}
			const std::string tif = AppPaths::joinPath(exportDir, "smoke_cmyk.tif");
			if (!live.exportImage(tif) || !std::filesystem::exists(tif)) {
				spdlog::error("pixelEditor smoke: CMYK TIFF export failed");
				return false;
			}
			const std::string cmykPsdIcc = AppPaths::joinPath(exportDir, "smoke_cmyk_icc.psd");
			if (!live.savePsd(cmykPsdIcc) || !live.loadPsd(cmykPsdIcc) || !live.canvas().isCmyk()) {
				spdlog::error("pixelEditor smoke: CMYK PSD (ICC path) failed");
				return false;
			}
			spdlog::info("pixelEditor smoke: CMYK mode + TIFF + PSD ok");

			const std::string pdfPath = AppPaths::joinPath(exportDir, "smoke_pdfx.pdf");
			if (!live.exportPdf(pdfPath) || !std::filesystem::exists(pdfPath)) {
				spdlog::error("pixelEditor smoke: Export PDF/X failed");
				return false;
			}
			{
				std::ifstream in(pdfPath, std::ios::binary);
				std::string bytes((std::istreambuf_iterator<char>(in)),
								  std::istreambuf_iterator<char>());
				if (bytes.find("%PDF") == std::string::npos) {
					spdlog::error("pixelEditor smoke: PDF missing %PDF header");
					return false;
				}
				if (bytes.find("/OutputIntent") == std::string::npos &&
					bytes.find("OutputIntent") == std::string::npos &&
					bytes.find("GTS_PDFX") == std::string::npos) {
					spdlog::error("pixelEditor smoke: PDF missing OutputIntent / PDF/X mark");
					return false;
				}
			}
			spdlog::info("pixelEditor smoke: PDF/X ok");

			if (!live.convertColorMode(rigkit::ecs::PixelColorMode::RGB)) {
				spdlog::error("pixelEditor smoke: CMYK to RGB convert failed");
				return false;
			}
		}
	}
	return true;
}

bool PixelEditorApp::runBatch() {
	namespace fs = std::filesystem;
	if (!m_plotter || !m_document) {
		spdlog::error("batch: missing plotter or document pack");
		return false;
	}
	if (m_recipePath.empty() || m_batchIn.empty() || m_batchOut.empty()) {
		spdlog::error("batch: need --recipe <file.rig> --in <dir> --out <dir>");
		return false;
	}
	if (!fs::is_regular_file(m_recipePath)) {
		spdlog::error("batch: recipe not found '{}'", m_recipePath);
		return false;
	}
	if (!fs::is_directory(m_batchIn)) {
		spdlog::error("batch: input folder not found '{}'", m_batchIn);
		return false;
	}
	std::error_code ec;
	fs::create_directories(m_batchOut, ec);
	if (ec) {
		spdlog::error("batch: cannot create output '{}': {}", m_batchOut, ec.message());
		return false;
	}

	// One .rigz smart object per scan: recipe Curves/Levels + original packed
	// under data/links/. Disk originals are never overwritten. Relay places
	// these packages; PNG/PDF export happens later from the picture book.
	static const char* kExt[] = {".png", ".jpg", ".jpeg", ".bmp", ".tif", ".tiff", ".tga", ".gif"};
	auto isStill = [&](const fs::path& p) {
		std::string ext = p.extension().string();
		for (auto& c : ext) {
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		for (const char* e : kExt) {
			if (ext == e) {
				return true;
			}
		}
		return false;
	};

	auto& doc = m_plotter->doc();
	auto& ecs = doc.ecs();
	int ok = 0;
	int fail = 0;
	for (const auto& entry : fs::directory_iterator(m_batchIn, ec)) {
		if (ec || !entry.is_regular_file(ec)) {
			continue;
		}
		const fs::path src = entry.path();
		if (!isStill(src)) {
			continue;
		}
		if (!doc.loadProject(*m_document, m_recipePath)) {
			spdlog::error("batch: failed to load recipe '{}'", m_recipePath);
			++fail;
			continue;
		}
		doc.canvas().sizeLocked = false;
		if (!doc.loadImage(src.string())) {
			spdlog::error("batch: load failed '{}'", src.string());
			++fail;
			continue;
		}
		doc.canvas().width = std::max(1, doc.working().w);
		doc.canvas().height = std::max(1, doc.working().h);
		doc.canvas().title = src.stem().string();
		doc.canvas().markAllDirty();
		if (!doc.render()) {
			spdlog::error("batch: render failed '{}'", src.string());
			++fail;
			continue;
		}

		const fs::path dest = fs::path(m_batchOut) / (src.stem().string() + ".rigz");
		const fs::path staging =
			fs::temp_directory_path() /
			("pixel-rigz-" +
			 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		fs::create_directories(staging, ec);
		if (ec) {
			spdlog::error("batch: cannot stage '{}'", staging.string());
			++fail;
			continue;
		}

		// Mirror Relay save: collect into the package, write .rig with package
		// paths, zip, then restore live absolute links (batch reloads next).
		std::vector<std::pair<entt::entity, std::string>> liveLinks;
		for (auto e : ecs.view<rigkit::ecs::CAssetRef>()) {
			liveLinks.emplace_back(e, ecs.getComponent<rigkit::ecs::CAssetRef>(e).path);
		}
		std::string liveMedia = doc.source().mediaPath;
		doc.ensureProjectEnvelope((staging / (src.stem().string() + ".rig")).string());
		doc.ensureSourceAsset();
		std::string collectErr;
		if (!rigkit::project::collectLinkedAssets(ecs, staging.string(), &collectErr)) {
			spdlog::error("batch: collect links failed '{}': {}", dest.string(), collectErr);
			fs::remove_all(staging, ec);
			++fail;
			continue;
		}
		auto pathFromAsset = [&](uint32_t id) -> std::string {
			if (id == 0) {
				return {};
			}
			const auto e = static_cast<entt::entity>(id);
			if (!ecs.registry().valid(e) || !ecs.hasComponent<rigkit::ecs::CAssetRef>(e)) {
				return {};
			}
			return ecs.getComponent<rigkit::ecs::CAssetRef>(e).path;
		};
		{
			auto& source = doc.source();
			const std::string packed = pathFromAsset(source.asset);
			if (!packed.empty()) {
				source.mediaPath = packed;
			}
		}
		for (auto e : doc.layerEntities()) {
			if (!ecs.hasComponent<rigkit::ecs::CPixelLayer>(e)) {
				continue;
			}
			auto& L = ecs.getComponent<rigkit::ecs::CPixelLayer>(e);
			const std::string img = pathFromAsset(L.image);
			if (!img.empty()) {
				L.imagePath = img;
			}
			const std::string mask = pathFromAsset(L.maskAsset);
			if (!mask.empty()) {
				L.maskPath = mask;
			}
		}

		const fs::path tempRig = staging / (src.stem().string() + ".rig");
		if (!m_document->serializer().save(ecs, tempRig.string())) {
			spdlog::error("batch: write staged .rig failed '{}'", tempRig.string());
			for (const auto& [e, path] : liveLinks) {
				if (ecs.registry().valid(e) && ecs.hasComponent<rigkit::ecs::CAssetRef>(e)) {
					ecs.getComponent<rigkit::ecs::CAssetRef>(e).path = path;
				}
			}
			doc.source().mediaPath = liveMedia;
			fs::remove_all(staging, ec);
			++fail;
			continue;
		}
		std::string zipErr;
		if (!rigkit::project::writeRigz(dest.string(), tempRig.string(), staging.string(),
										&zipErr)) {
			spdlog::error("batch: write .rigz failed '{}': {}", dest.string(), zipErr);
			for (const auto& [e, path] : liveLinks) {
				if (ecs.registry().valid(e) && ecs.hasComponent<rigkit::ecs::CAssetRef>(e)) {
					ecs.getComponent<rigkit::ecs::CAssetRef>(e).path = path;
				}
			}
			doc.source().mediaPath = liveMedia;
			fs::remove_all(staging, ec);
			++fail;
			continue;
		}
		for (const auto& [e, path] : liveLinks) {
			if (ecs.registry().valid(e) && ecs.hasComponent<rigkit::ecs::CAssetRef>(e)) {
				ecs.getComponent<rigkit::ecs::CAssetRef>(e).path = path;
			}
		}
		doc.source().mediaPath = liveMedia;
		fs::remove_all(staging, ec);
		spdlog::info("batch: {} -> {} (sidecar packed)", src.filename().string(), dest.string());
		++ok;
	}
	if (ok == 0 && fail == 0) {
		spdlog::error("batch: no still images in '{}'", m_batchIn);
		return false;
	}
	spdlog::info("batch: wrote {} .rigz smart object(s), {} failed", ok, fail);
	return fail == 0;
}
