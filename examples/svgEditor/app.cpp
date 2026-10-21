#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/IMui.h"
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
#include "packs/rigDxf/src/DxfLoader.h"
#include "packs/rigDxf/src/rigDxf.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigImage/src/Image.h"
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
#include "packs/rigSystems/src/rigSystems.h"
#include "packs/rigTrace/src/Trace.h"
#include "packs/rigTrace/src/TraceStack.h"
#include "packs/rigTrace/src/rigTrace.h"
#include "packs/rigTraceUi/src/rigTraceUi.h"
#include "packs/rigVectorEditor/src/PathBooleanOps.h"
#include "packs/rigVectorEditor/src/PathEditOps.h"
#include "packs/rigVectorEditor/src/PathOffsetOps.h"
#include "packs/rigVectorEditor/src/SoftMaskOps.h"
#include "packs/rigVectorEditor/src/TextOutlineOps.h"
#include "packs/rigVectorEditor/src/rigVectorEditor.h"
#include "packs/rigVectorUi/src/VectorDocumentCommands.h"
#include "packs/rigVectorUi/src/VectorEditUndo.h"
#include "packs/rigVectorUi/src/rigVectorUi.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace {

std::string tracePlateLayerName(const rigkit::trace::TracePlate& plate) {
	return rigkit::trace::plateInkName(plate);
}

} // namespace

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
	packs->registerPack<rigkit::rigDxf>();
	packs->registerPack<rigkit::rigVectorEditor>();
	packs->registerPack<rigkit::rigArtboards>();
	packs->registerPack<rigkit::rigImage>();
	packs->registerPack<rigkit::rigPlotter>();
	packs->registerPack<rigkit::rigImGui>();
	packs->registerPack<rigkit::rigVectorUi>();
	packs->registerPack<rigkit::rigTrace>();
	packs->registerPack<rigkit::rigTraceUi>();
	packs->initAll();
	packs->setupAll();

	m_document = packs->getPack<rigkit::rigProject>();
	m_plotter = packs->getPack<rigkit::rigPlotter>();
	m_traceUi = packs->getPack<rigkit::rigTraceUi>();
	if (auto vectorUi = packs->getPack<rigkit::rigVectorUi>()) {
		if (auto* cmd = vectorUi->documentCommands()) {
			cmd->registerPathLoader(".dxf",
									[](const std::string& path, rigkit::plot::ParsedPathDoc& out) {
										return rigkit::dxf::loadFile(path, out);
									});
		}
	}
	if (!m_document || !m_plotter) {
		spdlog::error("svgEditor - missing rigProject or rigPlotter after bootstrap");
		return;
	}
	if (m_traceUi && m_traceUi->panel()) {
		m_traceUi->panel()->setCommitHook([this](const rigkit::trace::TraceStack& stack) {
			if (!m_plotter || stack.empty()) {
				return;
			}
			auto* ui = m_engine ? m_engine->getUiManager() : nullptr;
			rigkit::UndoStack* undo = ui ? ui->undoStack() : nullptr;
			auto* doc = &m_plotter->doc();
			rigkit::mutateDocGeometry(undo, doc, "Trace image", [&] {
				if (!m_traceApplied.empty()) {
					const auto layers = doc->layerEntities();
					std::vector<int> drop;
					for (unsigned int id : m_traceApplied) {
						const auto e = static_cast<entt::entity>(id);
						for (int i = 0; i < static_cast<int>(layers.size()); ++i) {
							if (layers[static_cast<size_t>(i)] == e) {
								drop.push_back(i);
								break;
							}
						}
					}
					std::sort(drop.begin(), drop.end());
					drop.erase(std::unique(drop.begin(), drop.end()), drop.end());
					for (int k = static_cast<int>(drop.size()) - 1; k >= 0; --k) {
						doc->removeLayer(drop[static_cast<size_t>(k)]);
					}
					m_traceApplied.clear();
				}
				for (const auto& plate : stack.plates) {
					if (plate.empty() || !plate.included) {
						continue;
					}
					auto e = doc->addLayer(tracePlateLayerName(plate));
					if (e == entt::null) {
						continue;
					}
					rigkit::trace::TraceStack one;
					one.plates.push_back(plate);
					rigkit::ecs::CPaths bag;
					rigkit::trace::flattenStack(one, bag);
					doc->ecs().registry().emplace_or_replace<rigkit::ecs::CPaths>(e, bag);
					doc->markPathsGpuDirty(e);
					m_traceApplied.push_back(static_cast<unsigned int>(e));
				}
			});
			doc->bumpViewEpoch();
		});
	}

	auto& doc = m_plotter->doc();
	// Page stays SVG Y-down; the overlay sets CBedView.yUp = false.
	auto& opts = doc.importOptions();
	opts.svgFlipY = false;
	opts.dxfFlipY = true;
	opts.dxfOriginToPage = true;
	opts.dxfJoinConnected = true;
	opts.dxfPromoteFacetedCircles = true;
	opts.dxfPromoteFacetedArcs = true;
	opts.dxfImportText = true;
	opts.dxfImportDimensions = true;
	opts.dxfDimensionsOwnLayer = true;
	if (m_smoke) {
		// Assertions are written against the compact Letter/Blob document.
		const std::string demoPath = AppPaths::joinPath(AppPaths::getDataDir(), "demo.svg");
		if (doc.importSvg(demoPath)) {
			doc.meta().name = "demo";
			doc.meta().workingSvgPath = demoPath;
			doc.ensureProjectEnvelope(documentPathHint());
			doc.bumpViewEpoch();
		} else {
			spdlog::warn("svgEditor - demo.svg missing; seeding Letter/Blob layers");
			seedDemoLayers();
			doc.meta().name = "demo";
			doc.ensureProjectEnvelope(documentPathHint());
			doc.bumpViewEpoch();
		}
		doc.pathEdit().layerIndex = 0;
		doc.pathEdit().selectOnly(0);
		m_smokeOk = runSmoke();
		if (!m_smokeOk) {
			spdlog::error("svgEditor --smoke failed");
		} else {
			spdlog::info("svgEditor --smoke ok");
		}
		if (auto* win = m_engine->getWindow()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
		return;
	}

	// Interactive launch stays empty until File > New... or Open...
	doc.closeSession();
}

void SvgEditorApp::update(float) {
	syncTraceSource();
}

void SvgEditorApp::clearTraceSource() {
	if (m_traceUi && m_traceUi->panel()) {
		m_traceUi->panel()->clearSource();
	}
	m_traceHref.clear();
	m_traceLayer = -1;
	m_traceImage = -1;
	m_traceX = 0.f;
	m_traceY = 0.f;
	m_traceW = 0.f;
	m_traceH = 0.f;
}

void SvgEditorApp::syncTraceSource() {
	if (!m_traceUi || !m_traceUi->panel() || !m_plotter) {
		return;
	}
	auto& doc = m_plotter->doc();
	auto& sel = doc.pathEdit();
	if (sel.imageIndex < 0 || sel.layerIndex < 0) {
		if (m_traceImage >= 0) {
			clearTraceSource();
		}
		return;
	}
	const auto layers = doc.layerEntities();
	if (sel.layerIndex >= static_cast<int>(layers.size())) {
		if (m_traceImage >= 0) {
			clearTraceSource();
		}
		return;
	}
	const auto e = layers[static_cast<size_t>(sel.layerIndex)];
	if (!doc.ecs().hasComponent<rigkit::ecs::CImages>(e)) {
		if (m_traceImage >= 0) {
			clearTraceSource();
		}
		return;
	}
	const auto& images = doc.ecs().getComponent<rigkit::ecs::CImages>(e);
	if (sel.imageIndex >= static_cast<int>(images.items.size())) {
		if (m_traceImage >= 0) {
			clearTraceSource();
		}
		return;
	}
	const auto& item = images.items[static_cast<size_t>(sel.imageIndex)];
	if (sel.layerIndex == m_traceLayer && sel.imageIndex == m_traceImage &&
		item.href == m_traceHref && std::abs(item.pos.x - m_traceX) < 0.01f &&
		std::abs(item.pos.y - m_traceY) < 0.01f && std::abs(item.size.x - m_traceW) < 0.01f &&
		std::abs(item.size.y - m_traceH) < 0.01f) {
		return;
	}

	const auto& meta = doc.meta();
	const std::string& base = !meta.workingSvgPath.empty() ? meta.workingSvgPath : meta.sourcePath;
	std::string baseDir;
	if (!base.empty()) {
		baseDir = std::filesystem::path(base).parent_path().string();
	}
	const std::string resolved = rig::image::resolvePath(item.href, baseDir);
	rig::pixel::Raster pixels;
	if (resolved.empty() || !rig::image::load(resolved, pixels) || pixels.width <= 0) {
		clearTraceSource();
		return;
	}
	m_traceUi->panel()->setSource(pixels, item.pos.x, item.pos.y, item.size.x, item.size.y);
	m_traceUi->panel()->setSourceLabel(item.href);
	m_traceLayer = sel.layerIndex;
	m_traceImage = sel.imageIndex;
	m_traceHref = item.href;
	m_traceX = item.pos.x;
	m_traceY = item.pos.y;
	m_traceW = item.size.x;
	m_traceH = item.size.y;
}

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

	{
		auto* packs = m_engine ? m_engine->getPackManager() : nullptr;
		auto vectorUi = packs ? packs->getPack<rigkit::rigVectorUi>() : nullptr;
		auto* cmd = vectorUi ? vectorUi->documentCommands() : nullptr;
		if (!cmd) {
			spdlog::error("svgEditor smoke: document commands missing");
			return false;
		}
		const std::string demoPath = AppPaths::joinPath(AppPaths::getDataDir(), "demo.svg");
		const std::string fixture =
			AppPaths::joinPath(AppPaths::getDataDir(), "fixtures/roundtrip-fills.svg");
		const std::string dxfPath =
			AppPaths::joinPath(AppPaths::getDataDir(), "fixtures/sample.dxf");
		doc.closeSession();
		cmd->acceptDrop({demoPath});
		if (!cmd->sessionOpen()) {
			spdlog::error("svgEditor smoke: drop-open failed");
			return false;
		}
		const size_t opened = doc.layerEntities().size();
		if (opened == 0) {
			spdlog::error("svgEditor smoke: drop-open left no layers");
			return false;
		}
		if (std::filesystem::exists(fixture)) {
			if (!cmd->importPath(fixture) || doc.layerEntities().size() <= opened) {
				spdlog::error("svgEditor smoke: SVG import-on-open failed");
				return false;
			}
			vectorUi->undoStack().undo();
			if (doc.layerEntities().size() != opened) {
				spdlog::error("svgEditor smoke: SVG import undo failed");
				return false;
			}
		}
		if (std::filesystem::exists(dxfPath)) {
			if (!cmd->importPath(dxfPath) || doc.layerEntities().size() <= opened) {
				spdlog::error("svgEditor smoke: DXF import failed");
				return false;
			}
			const auto after = doc.layerEntities();
			const auto dxfLayer = after.back();
			if (!doc.ecs().hasComponent<rigkit::ecs::CPaths>(dxfLayer)) {
				spdlog::error("svgEditor smoke: DXF import left no paths");
				return false;
			}
			const auto& bag = doc.ecs().getComponent<rigkit::ecs::CPaths>(dxfLayer);
			if (bag.paths.size() != 1) {
				spdlog::error("svgEditor smoke: DXF join failed ({} paths, want 1)",
							  bag.paths.size());
				return false;
			}
			float minX = 1e9f;
			float minY = 1e9f;
			float maxX = -1e9f;
			float maxY = -1e9f;
			for (const auto& path : bag.paths) {
				for (const auto& cmdPath : path.commands) {
					if (cmdPath.type == rigkit::ecs::CPath::Cmd::Close) {
						continue;
					}
					minX = std::min(minX, cmdPath.p.x);
					minY = std::min(minY, cmdPath.p.y);
					maxX = std::max(maxX, cmdPath.p.x);
					maxY = std::max(maxY, cmdPath.p.y);
				}
			}
			if (minX < -0.05f || minY < -0.05f || minX > 0.5f || minY > 0.5f) {
				spdlog::error("svgEditor smoke: DXF origin off page ({}, {})", minX, minY);
				return false;
			}
			if (std::fabs((maxX - minX) - 40.f) > 0.5f || std::fabs((maxY - minY) - 30.f) > 0.5f) {
				spdlog::error("svgEditor smoke: DXF size {}x{} (want 40x30)", maxX - minX,
							  maxY - minY);
				return false;
			}
			if (!doc.ecs().hasComponent<rigkit::ecs::CTexts>(dxfLayer) ||
				doc.ecs().getComponent<rigkit::ecs::CTexts>(dxfLayer).items.size() != 1 ||
				doc.ecs().getComponent<rigkit::ecs::CTexts>(dxfLayer).items[0].text != "Plate") {
				spdlog::error("svgEditor smoke: DXF text missing");
				return false;
			}
			vectorUi->undoStack().undo();
			if (doc.layerEntities().size() != opened) {
				spdlog::error("svgEditor smoke: DXF import undo failed");
				return false;
			}
		} else {
			spdlog::error("svgEditor smoke: fixtures/sample.dxf missing");
			return false;
		}
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
		if (!bag || bag->paths.empty() || !bag->isFilled(0) || bag->pathColors.empty() ||
			bag->pathColors[0].r < 0.9f || bag->pathColors[0].g > 0.1f) {
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

	rigkit::ecs::CPaths offsetBag;
	pushRect(offsetBag, 0.f, 0.f, 40.f, 30.f);
	const auto offsetOut = rigkit::pathEdit::applyPathOffset(
		offsetBag, {0}, 2.f, rigkit::pathEdit::JoinStyle::Miter, 2.f, 0.25f, false);
	if (offsetOut.size() != 1 || offsetBag.paths.size() != 1) {
		spdlog::error("svgEditor smoke: Offset Path failed");
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
					std::abs(item.pos.y - 18.f) < 0.05f && std::abs(item.size.x - 50.f) < 0.05f &&
					std::abs(item.size.y - 30.f) < 0.05f && std::abs(item.opacity - 0.8f) < 0.01f) {
					found = true;
				}
			}
		}
		if (!found) {
			spdlog::error("svgEditor smoke: <image> did not survive SVG round-trip");
			return false;
		}
	}

	{
		rig::pixel::Raster square;
		square.width = 64;
		square.height = 64;
		square.rgba.assign(64 * 64 * 4, 255);
		for (int y = 12; y < 52; ++y) {
			for (int x = 12; x < 52; ++x) {
				std::uint8_t* p = square.rgba.data() + static_cast<size_t>((y * 64 + x) * 4);
				p[0] = 0;
				p[1] = 0;
				p[2] = 0;
				p[3] = 255;
			}
		}
		rigkit::trace::TraceSettings ts;
		ts.hueDetect = false;
		ts.inkCount = 2;
		ts.posterizeLevels = 2;
		ts.keepBackground = false;
		ts.dropBackground = true;
		rigkit::ecs::CPaths traced;
		const int n = rigkit::trace::run(square, ts, traced, 0.f, 0.f, 64.f, 64.f);
		int lines = 0;
		int cubics = 0;
		std::vector<glm::vec2> verts;
		for (const auto& path : traced.paths) {
			for (const auto& c : path.commands) {
				if (c.type == rigkit::ecs::CPath::Cmd::LineTo) {
					++lines;
				} else if (c.type == rigkit::ecs::CPath::Cmd::CubicTo) {
					++cubics;
				}
				if (c.type == rigkit::ecs::CPath::Cmd::MoveTo ||
					c.type == rigkit::ecs::CPath::Cmd::LineTo) {
					verts.push_back(c.p);
				}
			}
			break;
		}
		if (n <= 0 || cubics != 0 || verts.size() < 4) {
			spdlog::error("svgEditor smoke: square was not LineTos (paths={} lines={} cubics={})",
						  n, lines, cubics);
			return false;
		}
		if (glm::length(verts.back() - verts.front()) < 1e-3f) {
			verts.pop_back();
		}
		if (verts.size() != 4) {
			spdlog::error("svgEditor smoke: square vertex count {} (want 4)", verts.size());
			return false;
		}
		const float s0 = glm::length(verts[1] - verts[0]);
		const float s1 = glm::length(verts[2] - verts[1]);
		if (s0 < 1.f || std::abs(s0 - s1) / s0 > 0.08f) {
			spdlog::error("svgEditor smoke: sides not equal ({:.2f} vs {:.2f})", s0, s1);
			return false;
		}
		const glm::vec2 e0 = glm::normalize(verts[1] - verts[0]);
		const glm::vec2 e1 = glm::normalize(verts[2] - verts[1]);
		if (std::abs(glm::dot(e0, e1)) > 0.12f) {
			spdlog::error("svgEditor smoke: corner not square (dot={:.3f})", glm::dot(e0, e1));
			return false;
		}

		// Default panel path: hue bins + greys then merge down to inkCount.
		// Six flat inks, one merge (white into a neighbour) - not yellow into green.
		rig::pixel::Raster swatch;
		swatch.width = 48;
		swatch.height = 48;
		swatch.rgba.assign(48 * 48 * 4, 255);
		const std::uint8_t inks[][3] = {{220, 40, 40},	{40, 180, 60}, {40, 80, 210},
										{240, 220, 40}, {20, 20, 20},  {240, 240, 240}};
		for (int y = 0; y < 48; ++y) {
			for (int x = 0; x < 48; ++x) {
				const int band = (x / 8) % 6;
				std::uint8_t* p = swatch.rgba.data() + static_cast<size_t>((y * 48 + x) * 4);
				p[0] = inks[band][0];
				p[1] = inks[band][1];
				p[2] = inks[band][2];
			}
		}
		rigkit::trace::TraceSettings hueTs;
		hueTs.hueDetect = true;
		hueTs.inkCount = 5;
		hueTs.hueBins = 4;
		hueTs.posterizeLevels = 4;
		hueTs.keepBackground = true;
		hueTs.dropBackground = false;
		rigkit::trace::TraceStack hueStack;
		const int hn = rigkit::trace::runStack(swatch, hueTs, hueStack, 0.f, 0.f, 48.f, 48.f);
		if (hn < 5 || hueStack.plates.size() < 5 || !hueStack.plates.front().isField) {
			spdlog::error("svgEditor smoke: hue merge kept {} plates (need Field + 4 inks)", hn);
			return false;
		}

		// Nested accents (blue field, green body, pink dots, black + yellow).
		// Closest-pair RGB used to swallow the marks into green and drop the field.
		auto putRgb = [](rig::pixel::Raster& img, int x, int y, std::uint8_t r, std::uint8_t g,
						 std::uint8_t b) {
			if (x < 0 || y < 0 || x >= img.width || y >= img.height) {
				return;
			}
			std::uint8_t* p = img.rgba.data() + static_cast<size_t>((y * img.width + x) * 4);
			p[0] = r;
			p[1] = g;
			p[2] = b;
			p[3] = 255;
		};
		auto fillCircle = [&](rig::pixel::Raster& img, int cx, int cy, int rad, std::uint8_t r,
							  std::uint8_t g, std::uint8_t b) {
			const int r2 = rad * rad;
			for (int y = cy - rad; y <= cy + rad; ++y) {
				for (int x = cx - rad; x <= cx + rad; ++x) {
					const int dx = x - cx;
					const int dy = y - cy;
					if (dx * dx + dy * dy <= r2) {
						putRgb(img, x, y, r, g, b);
					}
				}
			}
		};
		auto fillRect = [&](rig::pixel::Raster& img, int x0, int y0, int x1, int y1, std::uint8_t r,
							std::uint8_t g, std::uint8_t b) {
			for (int y = y0; y < y1; ++y) {
				for (int x = x0; x < x1; ++x) {
					putRgb(img, x, y, r, g, b);
				}
			}
		};
		rig::pixel::Raster pad;
		pad.width = 160;
		pad.height = 120;
		pad.rgba.assign(160 * 120 * 4, 255);
		fillRect(pad, 0, 0, 160, 120, 70, 90, 200);
		fillCircle(pad, 52, 60, 34, 80, 190, 100);
		fillCircle(pad, 108, 60, 34, 80, 190, 100);
		fillRect(pad, 52, 26, 108, 94, 80, 190, 100);
		fillCircle(pad, 36, 38, 7, 240, 170, 170);
		fillCircle(pad, 124, 38, 7, 240, 170, 170);
		fillCircle(pad, 36, 82, 7, 240, 170, 170);
		fillCircle(pad, 124, 82, 7, 240, 170, 170);
		fillCircle(pad, 64, 60, 8, 20, 20, 20);
		fillRect(pad, 78, 54, 90, 66, 250, 220, 40);
		fillRect(pad, 92, 54, 104, 66, 245, 245, 230);
		fillCircle(pad, 114, 60, 8, 20, 20, 20);
		rigkit::trace::TraceSettings padTs = hueTs;
		padTs.hueBins = 5;
		padTs.satFloor = 0.12f;
		rigkit::trace::TraceStack padStack;
		const int pn = rigkit::trace::runStack(pad, padTs, padStack, 0.f, 0.f, 160.f, 120.f);
		int sawField = 0;
		int sawGreen = 0;
		int sawPink = 0;
		int sawBlack = 0;
		int sawYellow = 0;
		for (const auto& plate : padStack.plates) {
			if (plate.isField) {
				++sawField;
			}
			const std::string name = rigkit::trace::plateInkName(plate);
			if (name == "Green") {
				++sawGreen;
			} else if (name == "Red" || name == "Magenta") {
				++sawPink;
			} else if (name == "Black") {
				++sawBlack;
			} else if (name == "Yellow") {
				++sawYellow;
			}
		}
		if (pn < 4 || !sawField || !sawGreen || !sawPink || (!sawBlack && !sawYellow)) {
			spdlog::error("svgEditor smoke: nested accents collapsed (plates={} field={} green={} "
						  "pink={} black={} yellow={})",
						  pn, sawField, sawGreen, sawPink, sawBlack, sawYellow);
			return false;
		}

		// Bone + pink cap + five yellow dots. Select-colour outline, not a
		// page-wide capsule and not yellow eaten by the body.
		rig::pixel::Raster bone;
		bone.width = 160;
		bone.height = 100;
		bone.rgba.assign(160 * 100 * 4, 255);
		fillRect(bone, 0, 0, 160, 100, 70, 90, 200);
		fillCircle(bone, 50, 50, 30, 90, 55, 35);
		fillCircle(bone, 110, 50, 30, 90, 55, 35);
		fillRect(bone, 50, 20, 110, 80, 90, 55, 35);
		for (int y = 58; y < 82; ++y) {
			for (int x = 20; x < 140; ++x) {
				std::uint8_t* p = bone.rgba.data() + static_cast<size_t>((y * 160 + x) * 4);
				if (p[0] == 90 && p[1] == 55 && p[2] == 35) {
					p[0] = 230;
					p[1] = 140;
					p[2] = 170;
				}
			}
		}
		const int dots[5][2] = {{48, 36}, {70, 34}, {90, 34}, {110, 36}, {80, 48}};
		for (const auto& d : dots) {
			fillCircle(bone, d[0], d[1], 5, 250, 220, 40);
		}
		rigkit::trace::TraceSettings boneTs;
		boneTs.hueDetect = true;
		boneTs.inkCount = 5;
		boneTs.satFloor = 0.14f;
		boneTs.keepBackground = true;
		boneTs.dropBackground = false;
		boneTs.completeUnder = false;
		rigkit::trace::TraceStack boneStack;
		const int bn = rigkit::trace::runStack(bone, boneTs, boneStack, 0.f, 0.f, 160.f, 100.f);
		int boneField = 0;
		int boneYellowMoves = 0;
		int bonePink = 0;
		int boneBodyPath = 0;
		int boneCapsule = 0;
		for (const auto& plate : boneStack.plates) {
			if (plate.isField) {
				++boneField;
				continue;
			}
			const std::string name = rigkit::trace::plateInkName(plate);
			for (const auto& shape : plate.shapes) {
				if (shape.kind == rigkit::trace::ShapeKind::Capsule) {
					++boneCapsule;
				}
				if (name == "Yellow") {
					for (const auto& c : shape.path.commands) {
						if (c.type == rigkit::ecs::CPath::Cmd::MoveTo) {
							++boneYellowMoves;
						}
					}
				} else if (plate.color.r > 0.7f && plate.color.b > 0.45f && plate.color.g < 0.75f) {
					++bonePink;
				} else if (shape.kind == rigkit::trace::ShapeKind::Path) {
					++boneBodyPath;
				}
			}
		}
		if (bn < 4 || !boneField || boneYellowMoves < 5 || !bonePink || !boneBodyPath ||
			boneCapsule > 0) {
			spdlog::error("svgEditor smoke: bone outline failed (plates={} field={} yellowMoves={} "
						  "pink={} bodyPath={} capsule={})",
						  bn, boneField, boneYellowMoves, bonePink, boneBodyPath, boneCapsule);
			return false;
		}
		int yellowIdx = -1;
		glm::vec4 yellowCol{};
		for (int i = 0; i < static_cast<int>(boneStack.plates.size()); ++i) {
			const auto& plate = boneStack.plates[static_cast<size_t>(i)];
			if (!plate.isField && rigkit::trace::plateInkName(plate) == "Yellow") {
				yellowIdx = i;
				yellowCol = plate.color;
				break;
			}
		}
		if (yellowIdx < 0) {
			spdlog::error("svgEditor smoke: bone lock needs a Yellow plate");
			return false;
		}
		boneStack.plates[static_cast<size_t>(yellowIdx)].included = false;
		boneStack.plates[static_cast<size_t>(yellowIdx)].locked = true;
		boneTs.inkCount = 4;
		boneTs.satFloor = 0.22f;
		rigkit::trace::TraceStack lockedStack;
		const int ln = rigkit::trace::runStack(bone, boneTs, lockedStack, 0.f, 0.f, 160.f, 100.f,
											   nullptr, &boneStack);
		int lockedYellow = 0;
		int unlockedYellow = 0;
		for (const auto& plate : lockedStack.plates) {
			const bool sameCol = std::abs(plate.color.r - yellowCol.r) < 0.02f &&
								 std::abs(plate.color.g - yellowCol.g) < 0.02f &&
								 std::abs(plate.color.b - yellowCol.b) < 0.02f;
			const bool yellow =
				!plate.isField && (rigkit::trace::plateInkName(plate) == "Yellow" || sameCol);
			if (plate.locked && !plate.included) {
				if (!sameCol) {
					spdlog::error("svgEditor smoke: locked plate colour changed");
					return false;
				}
				++lockedYellow;
			} else if (yellow) {
				++unlockedYellow;
			}
		}
		if (ln < 2 || lockedYellow != 1 || unlockedYellow != 0) {
			spdlog::error("svgEditor smoke: locked-off yellow retune failed (plates={} locked={} "
						  "unlockedYellow={})",
						  ln, lockedYellow, unlockedYellow);
			return false;
		}
	}

	spdlog::info("svgEditor smoke: shell ops ok (paths={})", pathCount);
	return true;
}
