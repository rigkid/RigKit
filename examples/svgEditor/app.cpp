#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "packs/rigArtboards/src/rigArtboards.h"
#include "packs/rigColorspace/src/rigColorspace.h"
#include "packs/rigComponent/src/CPath.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigCompositor/src/LayerMaskOps.h"
#include "packs/rigCompositor/src/rigCompositor.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigImage/src/rigImage.h"
#include "packs/rigPlotComponent/src/CCompositePass.h"
#include "packs/rigPlotComponent/src/CImages.h"
#include "packs/rigPlotComponent/src/CPaths.h"
#include "packs/rigPlotComponent/src/CTexts.h"
#include "packs/rigPlotComponent/src/ParsedPathDoc.h"
#include "packs/rigPlotComponent/src/PathFlatten.h"
#include "packs/rigPlotComponent/src/rigPlotComponent.h"
#include "packs/rigPlotter/src/PlotDoc.h"
#include "packs/rigPlotter/src/rigPlotter.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigSvg/src/rigSvg.h"
#include "packs/rigSvgEditorUi/src/rigSvgEditorUi.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "packs/rigVectorEditor/src/PathBooleanOps.h"
#include "packs/rigVectorEditor/src/PathEditOps.h"
#include "packs/rigVectorEditor/src/SoftMaskOps.h"
#include "packs/rigVectorEditor/src/TextOutlineOps.h"
#include "packs/rigVectorEditor/src/rigVectorEditor.h"

#include <glm/glm.hpp>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>
#include <vector>

SvgEditorApp::SvgEditorApp() {
	window().width = 800;
	window().height = 600;
	window().title = "svgEditor";
}

void SvgEditorApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
}

void SvgEditorApp::seedDemoLayers() {
	if (!m_plotter) {
		return;
	}
	using Cmd = rigkit::ecs::CPath::Cmd;
	rigkit::plot::ParsedPathDoc parsed;
	parsed.contentW = 400.f;
	parsed.contentH = 300.f;
	parsed.minX = 0.f;
	parsed.minY = 0.f;

	{
		rigkit::plot::PathSourceLayer letter;
		letter.name = "Letter";
		letter.layerColor = {1.f, 0.824f, 0.275f, 1.f};
		rigkit::ecs::CPath path;
		path.commands.push_back({Cmd::MoveTo, {120.f, 170.f}, {}, {}});
		path.commands.push_back({Cmd::CubicTo, {78.f, 130.f}, {96.f, 170.f}, {78.f, 154.f}});
		path.commands.push_back({Cmd::CubicTo, {128.f, 88.f}, {78.f, 104.f}, {98.f, 88.f}});
		path.commands.push_back({Cmd::CubicTo, {168.f, 110.f}, {146.f, 88.f}, {160.f, 96.f}});
		path.commands.push_back({Cmd::LineTo, {168.f, 88.f}, {}, {}});
		path.commands.push_back({Cmd::LineTo, {190.f, 88.f}, {}, {}});
		path.commands.push_back({Cmd::LineTo, {190.f, 170.f}, {}, {}});
		path.commands.push_back({Cmd::LineTo, {170.f, 170.f}, {}, {}});
		path.commands.push_back({Cmd::LineTo, {170.f, 150.f}, {}, {}});
		path.commands.push_back({Cmd::CubicTo, {120.f, 170.f}, {162.f, 164.f}, {146.f, 170.f}});
		path.commands.push_back({Cmd::Close, {}, {}, {}});
		// Counter wound opposite to the outer ring - NonZero fill keeps it open.
		path.commands.push_back({Cmd::MoveTo, {128.f, 108.f}, {}, {}});
		path.commands.push_back({Cmd::CubicTo, {100.f, 130.f}, {112.f, 108.f}, {100.f, 116.f}});
		path.commands.push_back({Cmd::CubicTo, {128.f, 154.f}, {100.f, 144.f}, {112.f, 154.f}});
		path.commands.push_back({Cmd::CubicTo, {154.f, 128.f}, {142.f, 154.f}, {154.f, 144.f}});
		path.commands.push_back({Cmd::CubicTo, {128.f, 108.f}, {154.f, 116.f}, {142.f, 108.f}});
		path.commands.push_back({Cmd::Close, {}, {}, {}});
		letter.paths.paths.push_back(std::move(path));
		letter.paths.pathFilled.push_back(1);
		letter.paths.pathColors.push_back({1.f, 0.824f, 0.275f, 1.f});
		letter.paths.pathStroked.push_back(1);
		letter.paths.pathStrokeColors.push_back({1.f, 0.824f, 0.275f, 1.f});
		letter.paths.pathStrokeWidths.push_back(1.5f);
		parsed.layers.push_back(std::move(letter));
	}

	{
		rigkit::plot::PathSourceLayer blob;
		blob.name = "Blob";
		blob.layerColor = {0.353f, 0.784f, 1.f, 1.f};
		rigkit::ecs::CPath path;
		constexpr float cx = 300.f, cy = 150.f, r = 55.f;
		constexpr float k = 0.5522847498f;
		const float kx = r * k, ky = r * k;
		path.commands.push_back({Cmd::MoveTo, {cx + r, cy}, {}, {}});
		path.commands.push_back({Cmd::CubicTo, {cx, cy + r}, {cx + r, cy + ky}, {cx + kx, cy + r}});
		path.commands.push_back({Cmd::CubicTo, {cx - r, cy}, {cx - kx, cy + r}, {cx - r, cy + ky}});
		path.commands.push_back({Cmd::CubicTo, {cx, cy - r}, {cx - r, cy - ky}, {cx - kx, cy - r}});
		path.commands.push_back({Cmd::CubicTo, {cx + r, cy}, {cx + kx, cy - r}, {cx + r, cy - ky}});
		path.commands.push_back({Cmd::Close, {}, {}, {}});
		blob.paths.paths.push_back(std::move(path));
		blob.paths.pathFilled.push_back(0);
		blob.paths.pathStroked.push_back(1);
		blob.paths.pathStrokeColors.push_back({0.353f, 0.784f, 1.f, 1.f});
		blob.paths.pathStrokeWidths.push_back(2.f);
		parsed.layers.push_back(std::move(blob));
	}

	m_plotter->doc().commitParsed(parsed, true);
}

void SvgEditorApp::setup() {
	spdlog::info("svgEditor - SVG editor shell (toolbar / artboard / layers)");
	m_engine->setClearColor(0.07f, 0.07f, 0.086f, 1.0f);

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
	packs->registerPack<rigkit::rigVectorEditor>();
	packs->registerPack<rigkit::rigArtboards>();
	packs->registerPack<rigkit::rigImage>();
	packs->registerPack<rigkit::rigPlotter>();
	packs->registerPack<rigkit::rigImGui>();
	packs->registerPack<rigkit::rigSvgEditorUi>();
	packs->initAll();
	packs->setupAll();

	m_document = packs->getPack<rigkit::rigProject>();
	m_plotter = packs->getPack<rigkit::rigPlotter>();
	if (!m_document || !m_plotter) {
		spdlog::error("svgEditor - missing rigProject or rigPlotter after bootstrap");
		return;
	}

	auto& doc = m_plotter->doc();
	// Page stays SVG Y-down; the overlay sets CBedView.yUp = false.
	doc.importOptions().svgFlipY = false;
	// Letter/Blob demo is the sample; showcase.svg is File > Samples.
	// --smoke uses the same compact document its assertions are written against.
	const std::string dataDir = AppPaths::getDataDir();
	const std::string demoPath = dataDir + "/demo.svg";
	if (doc.importSvg(demoPath)) {
		doc.meta().name = "demo";
		doc.meta().workingSvgPath = demoPath;
		doc.ensureProjectEnvelope(documentPathHint());
		doc.bumpViewEpoch();
		spdlog::info("svgEditor - imported {} (paper {:.0f}x{:.0f} mm, {} layers)", demoPath,
					 doc.pageSize().x, doc.pageSize().y, doc.layerEntities().size());
	} else {
		spdlog::warn("svgEditor - demo.svg missing; seeding Letter/Blob layers");
		seedDemoLayers();
		doc.meta().name = "demo";
		doc.ensureProjectEnvelope(AppPaths::joinPath(AppPaths::getDataDir(), "demo"));
		doc.bumpViewEpoch();
	}

	// Preselect letter path on first run.
	doc.pathEdit().layerIndex = 0;
	doc.pathEdit().selectOnly(0);

	if (m_smoke) {
		m_smokeOk = runSmoke();
		if (!m_smokeOk) {
			spdlog::error("svgEditor --smoke failed");
		} else {
			spdlog::info("svgEditor --smoke ok");
		}
		if (auto* win = m_engine->getWindow()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
	}
}

void SvgEditorApp::update(float) {}

std::string SvgEditorApp::documentPathHint() const {
	return AppPaths::joinPath(AppPaths::getDataDir(), "demo");
}

bool SvgEditorApp::runSmoke() {
	if (!m_plotter || !m_document) {
		return false;
	}
	auto& doc = m_plotter->doc();
	const auto layers = doc.layerEntities();
	if (layers.size() < 2) {
		spdlog::error("svgEditor smoke: expected Letter+Blob layers, got {}", layers.size());
		return false;
	}
	size_t pathCount = 0;
	for (auto e : layers) {
		if (doc.ecs().hasComponent<rigkit::ecs::CPaths>(e)) {
			pathCount += doc.ecs().getComponent<rigkit::ecs::CPaths>(e).paths.size();
		}
	}
	if (pathCount == 0) {
		spdlog::error("svgEditor smoke: no paths");
		return false;
	}

	const std::string exportDir = AppPaths::joinPath(AppPaths::getDataDir(), "export");
	std::filesystem::create_directories(exportDir);
	const std::string rigPath = AppPaths::joinPath(exportDir, "smoke.rig");
	doc.ensureProjectEnvelope(rigPath);
	if (!doc.saveProject(*m_document, rigPath) || !doc.loadProject(*m_document, rigPath)) {
		spdlog::error("svgEditor smoke: .rig round-trip failed");
		return false;
	}

	const std::string fixture =
		AppPaths::joinPath(AppPaths::getDataDir(), "fixtures/roundtrip-fills.svg");
	if (std::filesystem::exists(fixture)) {
		if (!doc.importSvg(fixture)) {
			spdlog::error("svgEditor smoke: fixture import failed");
			return false;
		}
		auto* bag = doc.editablePaths();
		doc.pathEdit().layerIndex = 0;
		if (!bag || bag->paths.empty() ||
			!rigkit::pathEdit::setPathFillColor(*bag, 0, {1.f, 0.f, 0.f, 1.f})) {
			spdlog::error("svgEditor smoke: paint failed");
			return false;
		}
		const std::string svgOut = AppPaths::joinPath(exportDir, "smoke-roundtrip.svg");
		if (!doc.exportSvg(svgOut, rigkit::PlotDoc::SvgExportStyle::Authored) ||
			!doc.importSvg(svgOut)) {
			spdlog::error("svgEditor smoke: SVG round-trip failed");
			return false;
		}
		// Authored export is WYSIWYG: the red fill must survive the round-trip
		// as a fill, not come back as a stroke strip.
		doc.pathEdit().layerIndex = 0;
		bag = doc.editablePaths();
		if (!bag || bag->paths.empty() || !bag->isFilled(0) ||
			bag->pathColors.empty() || bag->pathColors[0].r < 0.9f ||
			bag->pathColors[0].g > 0.1f) {
			spdlog::error("svgEditor smoke: authored SVG export lost the fill");
			return false;
		}
	}

	doc.pathEdit().layerIndex = 0;
	auto* bag = doc.editablePaths();
	if (!bag) {
		return false;
	}
	const std::string font = AppPaths::joinPath(AppPaths::getFontsDir(), "Roboto-Regular.ttf");
	if (std::filesystem::exists(font)) {
		auto* texts = doc.editableTexts();
		if (!texts) {
			spdlog::error("svgEditor smoke: editableTexts failed");
			return false;
		}
		rigkit::ecs::TextItem item;
		item.text = "Ag";
		item.size = 18.f;
		item.baseline = {10.f, 40.f};
		texts->items.push_back(item);
		doc.pathEdit().selectText(0, 0);
		const std::string rigText = exportDir + "/smoke-text.rig";
		doc.ensureProjectEnvelope(rigText);
		if (!doc.saveProject(*m_document, rigText) || !doc.loadProject(*m_document, rigText)) {
			spdlog::error("svgEditor smoke: text .rig round-trip failed");
			return false;
		}
		texts = doc.editableTexts();
		if (!texts || texts->items.empty() || texts->items[0].text != "Ag") {
			spdlog::error("svgEditor smoke: text did not reload");
			return false;
		}

		// Format 1.0 documents wrote "sizeMm" / "baselineMm"; the reader must
		// still accept them. Rewrite the saved doc to the legacy keys and load.
		{
			std::ifstream in(rigText, std::ios::binary);
			std::string body((std::istreambuf_iterator<char>(in)),
							 std::istreambuf_iterator<char>());
			in.close();
			auto replaceAll = [](std::string& s, const std::string& from, const std::string& to) {
				for (size_t p = s.find(from); p != std::string::npos;
					 p = s.find(from, p + to.size())) {
					s.replace(p, from.size(), to);
				}
			};
			// Exact-value match: other components (x.rigkit.artboard) also
			// carry a "size" key and must keep it.
			replaceAll(body, "\"size\": 18.0", "\"sizeMm\": 18.0");
			replaceAll(body, "\"baseline\":", "\"baselineMm\":");
			replaceAll(body, "\"format_minor\": 1", "\"format_minor\": 0");
			const std::string legacyPath = exportDir + "/smoke-text-legacy.rig";
			std::ofstream out(legacyPath, std::ios::binary);
			out << body;
			out.close();
			if (!doc.loadProject(*m_document, legacyPath)) {
				spdlog::error("svgEditor smoke: legacy text .rig load failed");
				return false;
			}
			doc.pathEdit().layerIndex = 0;
			texts = doc.editableTexts();
			if (!texts || texts->items.empty() || std::abs(texts->items[0].size - 18.f) > 0.01f ||
				std::abs(texts->items[0].baseline.y - 40.f) > 0.01f) {
				spdlog::error("svgEditor smoke: legacy sizeMm/baselineMm keys did not read");
				return false;
			}
		}
		bag = doc.editablePaths();
		if (!bag) {
			return false;
		}
		const int before = static_cast<int>(bag->paths.size());
		const int textN = rigkit::pathEdit::shapeText(font, texts->items[0], *bag);
		if (textN != 2) {
			spdlog::error("svgEditor smoke: convert outlines expected 2 glyphs, got {}", textN);
			return false;
		}
		texts->items.clear();
		(void)before;
	} else {
		const int textN = rigkit::pathEdit::makeTextOutlines(font, 18.f, "Ag", {10.f, 40.f}, *bag);
		if (textN <= 0 && std::filesystem::exists(font)) {
			spdlog::error("svgEditor smoke: text outlines failed");
			return false;
		}
	}

	rigkit::ecs::CPaths boolBag;
	auto pushRect = [](rigkit::ecs::CPaths& dest, float x0, float y0, float x1, float y1) {
		using Cmd = rigkit::ecs::CPath::Cmd;
		rigkit::ecs::CPath path;
		path.commands.push_back({Cmd::MoveTo, {x0, y0}, {}, {}});
		path.commands.push_back({Cmd::LineTo, {x1, y0}, {}, {}});
		path.commands.push_back({Cmd::LineTo, {x1, y1}, {}, {}});
		path.commands.push_back({Cmd::LineTo, {x0, y1}, {}, {}});
		path.commands.push_back({Cmd::Close, {}, {}, {}});
		const int idx = static_cast<int>(dest.paths.size());
		dest.paths.push_back(std::move(path));
		rigkit::pathEdit::setPathFillColor(dest, idx, {0.3f, 0.5f, 0.9f, 1.f});
	};
	pushRect(boolBag, 0.f, 0.f, 40.f, 30.f);
	pushRect(boolBag, 20.f, 10.f, 60.f, 40.f);
	if (!rigkit::pathEdit::applyPathBoolean(boolBag, {0, 1},
											rigkit::pathEdit::PathBooleanOp::Unite)) {
		spdlog::error("svgEditor smoke: Unite failed");
		return false;
	}

	rigkit::ecs::CPaths maskBag;
	const int circ = rigkit::pathEdit::appendFilledCircle(maskBag, {50.f, 50.f}, 40.f);
	const auto mask = rigkit::plot::flattenPath(maskBag.paths[static_cast<size_t>(circ)], 0.5f);
	std::vector<glm::vec2> grid;
	for (float y = 42.f; y <= 58.f; y += 4.f) {
		for (float x = 42.f; x <= 58.f; x += 4.f) {
			grid.push_back({x, y});
		}
	}
	if (rigkit::pathEdit::maskCoverageRatio(grid, mask, false) < 0.9f) {
		spdlog::error("svgEditor smoke: mask coverage failed");
		return false;
	}

	{
		auto& reg = doc.ecs().registry();
		const auto layers2 = doc.layerEntities();
		if (!layers2.empty()) {
			auto* editBag = doc.editablePaths();
			if (editBag) {
				const int mi = rigkit::pathEdit::appendFilledCircle(*editBag, {100.f, 100.f}, 30.f);
				rigkit::compositor::setPathLayerMask(reg, layers2[0], mi);
				if (!rigkit::compositor::hasPathLayerMask(reg, layers2[0])) {
					spdlog::error("svgEditor smoke: LayerMaskOps set failed");
					return false;
				}
				if (!reg.all_of<rigkit::ecs::CCompositePass>(layers2[0])) {
					spdlog::error("svgEditor smoke: CCompositePass missing after mask");
					return false;
				}
				rigkit::compositor::clearLayerMask(reg, layers2[0]);
			}
		}
	}

	// Placed images: .rig round-trip, SVG <image> round-trip, page size survives export.
	{
		doc.pathEdit().layerIndex = 0;
		auto* images = doc.editableImages();
		if (!images) {
			spdlog::error("svgEditor smoke: editableImages failed");
			return false;
		}
		rigkit::ecs::ImageItem img;
		img.href = "images/smoke.png";
		img.pos = {12.f, 18.f};
		img.size = {50.f, 30.f};
		img.opacity = 0.8f;
		images->items.push_back(img);

		const std::string rigImg = exportDir + "/smoke-image.rig";
		doc.ensureProjectEnvelope(rigImg);
		if (!doc.saveProject(*m_document, rigImg) || !doc.loadProject(*m_document, rigImg)) {
			spdlog::error("svgEditor smoke: image .rig round-trip failed");
			return false;
		}
		doc.pathEdit().layerIndex = 0;
		images = doc.editableImages();
		if (!images || images->items.empty() || images->items[0].href != "images/smoke.png" ||
			std::abs(images->items[0].pos.x - 12.f) > 0.01f ||
			std::abs(images->items[0].size.x - 50.f) > 0.01f) {
			spdlog::error("svgEditor smoke: image did not reload from .rig");
			return false;
		}

		const glm::vec2 paperBefore = doc.pageSize();
		const std::string svgImg = exportDir + "/smoke-image.svg";
		if (!doc.exportSvg(svgImg, rigkit::PlotDoc::SvgExportStyle::Authored) ||
			!doc.importSvg(svgImg)) {
			spdlog::error("svgEditor smoke: image SVG round-trip failed");
			return false;
		}
		const glm::vec2 paperAfter = doc.pageSize();
		if (std::abs(paperAfter.x - paperBefore.x) > 0.5f ||
			std::abs(paperAfter.y - paperBefore.y) > 0.5f) {
			spdlog::error("svgEditor smoke: page size lost in SVG export ({}x{} to {}x{})",
						  paperBefore.x, paperBefore.y, paperAfter.x, paperAfter.y);
			return false;
		}
		bool found = false;
		for (auto e : doc.layerEntities()) {
			if (!doc.ecs().hasComponent<rigkit::ecs::CImages>(e)) {
				continue;
			}
			for (const auto& item : doc.ecs().getComponent<rigkit::ecs::CImages>(e).items) {
				if (item.href == "images/smoke.png" && std::abs(item.pos.x - 12.f) < 0.05f &&
					std::abs(item.pos.y - 18.f) < 0.05f &&
					std::abs(item.size.x - 50.f) < 0.05f &&
					std::abs(item.size.y - 30.f) < 0.05f &&
					std::abs(item.opacity - 0.8f) < 0.01f) {
					found = true;
				}
			}
		}
		if (!found) {
			spdlog::error("svgEditor smoke: <image> did not survive SVG round-trip");
			return false;
		}
	}

	spdlog::info("svgEditor smoke: shell ops ok (paths={})", pathCount);
	return true;
}
