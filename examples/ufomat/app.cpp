#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "ecs/MEcs.h"
#include "packs/rigArtboards/src/SurfaceChrome.h"
#include "packs/rigArtboards/src/SurfaceSeam.h"
#include "packs/rigArtboards/src/SurfaceView.h"
#include "packs/rigArtboards/src/rigArtboards.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigFont/src/rigFont.h"
#include "packs/rigFontComponent/src/rigFontComponent.h"
#include "packs/rigImGui/src/IWindow.h"
#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/UiDpi.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigPlotComponent/src/PathFlatten.h"
#include "packs/rigPlotComponent/src/rigPlotComponent.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "packs/rigUfo/src/rigUfo.h"
#include "packs/rigVectorEditor/src/PathEditOps.h"
#include "packs/rigVectorEditor/src/rigVectorEditor.h"
#include "CCode.h"
#include "CFontAxis.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CFontGroup.h"
#include "CFontKern.h"
#include "CName.h"
#include "CPath.h"
#include "CSelection.h"
#include "ContractImport.h"
#include "FontFind.h"
#include "FontShape.h"
#include "UfoIo.h"
#include "UfoRoundtrip.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

namespace {

void clearSelection(rigkit::MEcs& ecs) {
	for (auto e : ecs.registry().view<rigkit::ecs::CSelection>()) {
		ecs.registry().get<rigkit::ecs::CSelection>(e).isSelected = false;
	}
}

class GlyphListPanel : public rigkit::IWindow {
  public:
	explicit GlyphListPanel(UfomatApp* app) : IWindow("Glyphs"), m_app(app) {}

	void renderContents() override {
		auto* engine = getEngine();
		auto* ecs = engine ? engine->getECSManager() : nullptr;
		if (!ecs || !m_app) {
			return;
		}
		if (ImGui::Button("Open")) {
			m_app->promptOpen();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			m_app->promptSave();
		}
		ImGui::Separator();
		for (auto e : ecs->registry().view<rigkit::ecs::CFontFace>()) {
			const auto& face = ecs->registry().get<rigkit::ecs::CFontFace>(e);
			std::string label = face.family.empty() ? "Face" : face.family;
			if (!face.style.empty()) {
				label += " " + face.style;
			}
			if (ImGui::Selectable(label.c_str(), e == m_app->selectedFace())) {
				m_app->selectFace(e);
			}
		}
		ImGui::Separator();
		for (auto e : ecs->registry().view<rigkit::ecs::CFontGlyph>()) {
			const auto& g = ecs->registry().get<rigkit::ecs::CFontGlyph>(e);
			std::string label = ecs->entityName(e);
			if (label.empty() && ecs->hasComponent<rigkit::ecs::CName>(e)) {
				label = ecs->getComponent<rigkit::ecs::CName>(e).stableId;
			}
			if (!g.unicodes.empty()) {
				label += "  U+";
				char hex[16];
				std::snprintf(hex, sizeof(hex), "%04X", g.unicodes[0]);
				label += hex;
			}
			const bool selected = (e == m_app->selectedGlyph());
			if (ImGui::Selectable(label.c_str(), selected)) {
				m_app->selectGlyph(e);
			}
		}
	}

  private:
	UfomatApp* m_app = nullptr;
};

class GlyphSurfacePanel : public rigkit::IWindow {
  public:
	explicit GlyphSurfacePanel(UfomatApp* app)
		: IWindow("Glyph", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
							   ImGuiWindowFlags_NoScrollWithMouse),
		  m_app(app) {}

	void renderContents() override {
		auto* engine = getEngine();
		auto* ecs = engine ? engine->getECSManager() : nullptr;
		auto* ui = engine ? engine->getUiManager() : nullptr;
		if (!ecs || !m_app) {
			ImGui::TextUnformatted("No ECS");
			return;
		}
		const auto glyph = m_app->selectedGlyph();
		if (glyph == entt::null || !ecs->hasComponent<rigkit::ecs::CFontGlyph>(glyph)) {
			ImGui::TextUnformatted("Select a glyph");
			return;
		}
		if (!ecs->hasComponent<rigkit::ecs::CPath>(glyph)) {
			ecs->addComponent(glyph, rigkit::ecs::CPath{});
		}
		auto& path = ecs->getComponent<rigkit::ecs::CPath>(glyph);
		const auto& g = ecs->getComponent<rigkit::ecs::CFontGlyph>(glyph);
		auto& view = m_app->bedView();
		auto& board = m_app->board();

		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const float width = std::max(rigkit::uiPx(80.f), avail.x);
		const float height = std::max(rigkit::uiPx(80.f), avail.y);
		if (width > rigkit::uiPx(120.f) && height > rigkit::uiPx(120.f) &&
			(std::fabs(width - m_fitW) > 1.f || std::fabs(height - m_fitH) > 1.f)) {
			m_surface.fit(
				view, ImVec2(width, height),
				{board.pos.x, board.pos.y, board.pos.x + board.size.x, board.pos.y + board.size.y},
				style.WindowPadding.x);
			m_fitW = width;
			m_fitH = height;
		}

		ImGui::InvisibleButton("glyph_canvas", rigkit::uiHitSize(ImVec2(width, height)),
							   ImGuiButtonFlags_MouseButtonLeft |
								   ImGuiButtonFlags_MouseButtonMiddle);
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();
		ImGuiIO& io = ImGui::GetIO();
		const bool middle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

		rigkit::artboards::SurfaceMap map;
		map.canvasPos = canvasPos;
		map.view = &view;
		const glm::vec2 mouse = map.toMm(io.MousePos);
		const glm::vec2 localPx{io.MousePos.x - canvasPos.x, io.MousePos.y - canvasPos.y};

		if (hovered && io.MouseWheel != 0.f) {
			m_surface.handleWheel(view, localPx, io.MouseWheel);
		}
		if (active && middle) {
			m_surface.handlePan(view, {io.MouseDelta.x, io.MouseDelta.y});
		}

		if (hovered && !middle && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			m_snapPath = path;
			m_snapGlyph = g;
			m_haveSnap = true;
			const float tol = std::max(4.f, 8.f / std::max(0.05f, view.scalePxPerMm));
			if (rigkit::pathEdit::pickHit(path, mouse, tol, m_cmd, m_part)) {
				m_lastMouse = mouse;
				m_dragging = true;
			} else {
				m_cmd = -1;
				m_part = rigkit::ecs::PathEditPart::None;
				m_dragging = false;
			}
		}
		if (active && m_dragging && !middle && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			const glm::vec2 delta = mouse - m_lastMouse;
			rigkit::pathEdit::moveHit(path, m_cmd, m_part, delta, io.KeyAlt, io.KeyShift);
			m_lastMouse = mouse;
		}
		if (ImGui::IsItemDeactivated()) {
			if (m_haveSnap && m_dragging && ui) {
				if (auto* stack = ui->undoStack()) {
					struct Snap {
						rigkit::ecs::CPath path;
						rigkit::ecs::CFontGlyph glyph;
					};
					const Snap before{m_snapPath, m_snapGlyph};
					const Snap after{path, g};
					const auto entity = glyph;
					stack->pushSnapshot(
						"Edit outline", before, after, [engine, entity](const Snap& s) {
							auto* live = engine ? engine->getECSManager() : nullptr;
							if (!live || entity == entt::null) {
								return;
							}
							if (live->hasComponent<rigkit::ecs::CPath>(entity)) {
								live->getComponent<rigkit::ecs::CPath>(entity) = s.path;
							}
							if (live->hasComponent<rigkit::ecs::CFontGlyph>(entity)) {
								live->getComponent<rigkit::ecs::CFontGlyph>(entity) = s.glyph;
							}
						});
				}
			}
			m_dragging = false;
			m_haveSnap = false;
		}

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 canvasMax(canvasPos.x + width, canvasPos.y + height);
		dl->AddRectFilled(canvasPos, canvasMax, ImGui::GetColorU32(ImGuiCol_ChildBg),
						  style.FrameRounding);
		dl->AddRect(canvasPos, canvasMax, ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding);
		dl->PushClipRect(canvasPos, canvasMax, true);

		const std::vector<const rigkit::ecs::CArtboard*> boards{&board};
		m_surface.drawArtboards(dl, canvasPos, view, boards, true);
		if (board.showGrid) {
			m_surface.drawGrid(
				dl, canvasPos, view,
				{board.pos.x, board.pos.y, board.pos.x + board.size.x, board.pos.y + board.size.y},
				board.gridStepMm, ImGui::GetColorU32(ImGuiCol_Border, 0.28f));
		}

		auto toScreen = [&](const glm::vec2& mm) {
			const glm::vec2 px = view.mmToPx(mm);
			return ImVec2(canvasPos.x + px.x, canvasPos.y + px.y);
		};

		float upm = 1000.f;
		float asc = 800.f;
		float desc = -200.f;
		float cap = 700.f;
		float xh = 500.f;
		for (auto e : ecs->registry().view<rigkit::ecs::CFontFace>()) {
			const auto& f = ecs->registry().get<rigkit::ecs::CFontFace>(e);
			upm = f.unitsPerEm;
			asc = f.ascender;
			desc = f.descender;
			cap = f.capHeight;
			xh = f.xHeight;
			break;
		}
		const ImU32 guide = ImGui::GetColorU32(ImGuiCol_Text, 0.28f);
		const ImU32 baseCol = ImGui::GetColorU32(ImGuiCol_Text, 0.55f);
		const float x0 = 0.f;
		const float x1 = std::max(g.width, upm);
		auto hline = [&](float y, ImU32 col) {
			dl->AddLine(toScreen({x0, y}), toScreen({x1, y}), col, 1.f);
		};
		hline(0.f, baseCol);
		hline(asc, guide);
		hline(desc, guide);
		hline(cap, guide);
		hline(xh, guide);
		dl->AddLine(toScreen({0.f, desc}), toScreen({0.f, asc}), guide, 1.f);
		dl->AddLine(toScreen({g.width, desc}), toScreen({g.width, asc}),
					ImGui::GetColorU32(ImGuiCol_SliderGrabActive, 0.7f), 1.5f);

		const auto poly = rigkit::plot::flattenPath(path, 2.f);
		const ImU32 pathCol = ImGui::GetColorU32(ImGuiCol_Text, 0.9f);
		for (size_t i = 1; i < poly.size(); ++i) {
			dl->AddLine(toScreen(poly[i - 1]), toScreen(poly[i]), pathCol, rigkit::uiPx(2.f));
		}
		rigkit::artboards::drawPathAnchors(dl, map, path, m_cmd);
		dl->PopClipRect();
	}

  private:
	UfomatApp* m_app = nullptr;
	rigkit::artboards::SurfaceView m_surface;
	rigkit::ecs::CPath m_snapPath;
	rigkit::ecs::CFontGlyph m_snapGlyph;
	bool m_haveSnap = false;
	bool m_dragging = false;
	int m_cmd = -1;
	rigkit::ecs::PathEditPart m_part = rigkit::ecs::PathEditPart::None;
	glm::vec2 m_lastMouse{0.f};
	float m_fitW = 0.f;
	float m_fitH = 0.f;
};

std::string glyphLabel(rigkit::MEcs& ecs, entt::entity e) {
	std::string label = rigkit::font::stableName(ecs, e);
	if (label.empty()) {
		label = ecs.entityName(e);
	}
	return label;
}

class KernPanel : public rigkit::IWindow {
  public:
	explicit KernPanel(UfomatApp* app) : IWindow("Kern"), m_app(app) {}

	void renderContents() override {
		auto* engine = getEngine();
		auto* ecs = engine ? engine->getECSManager() : nullptr;
		if (!ecs || !m_app) {
			return;
		}
		std::vector<std::string> lefts;
		std::vector<std::string> rights;
		for (auto e : ecs->registry().view<rigkit::ecs::CFontGlyph>()) {
			const std::string n = glyphLabel(*ecs, e);
			if (n.empty()) {
				continue;
			}
			lefts.push_back(n);
			rights.push_back(n);
		}
		for (auto e : ecs->registry().view<rigkit::ecs::CFontGroup>()) {
			const auto& g = ecs->registry().get<rigkit::ecs::CFontGroup>(e);
			const std::string n = glyphLabel(*ecs, e);
			if (n.empty()) {
				continue;
			}
			if (g.side != rigkit::ecs::CFontGroup::Right) {
				lefts.push_back(n);
			}
			if (g.side != rigkit::ecs::CFontGroup::Left) {
				rights.push_back(n);
			}
		}
		if (lefts.empty() || rights.empty()) {
			ImGui::TextUnformatted("No glyphs");
			return;
		}
		if (!ImGui::BeginTable("kern", static_cast<int>(rights.size()) + 1,
							   ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
								   ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg,
							   ImVec2(0.f, 0.f))) {
			return;
		}
		ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthFixed, 96.f);
		for (const auto& r : rights) {
			ImGui::TableSetupColumn(r.c_str(), ImGuiTableColumnFlags_WidthFixed, 64.f);
		}
		ImGui::TableHeadersRow();
		for (const auto& left : lefts) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(left.c_str());
			for (size_t i = 0; i < rights.size(); ++i) {
				ImGui::TableSetColumnIndex(static_cast<int>(i + 1));
				const auto pair = rigkit::font::findKernEntity(*ecs, left, rights[i]);
				float v = 0.f;
				if (pair != entt::null) {
					v = ecs->getComponent<rigkit::ecs::CFontKern>(pair).value;
				}
				ImGui::PushID((left + "/" + rights[i]).c_str());
				ImGui::SetNextItemWidth(-1.f);
				if (ImGui::InputFloat("##k", &v, 0.f, 0.f, "%.0f")) {
					rigkit::font::setKernPair(*ecs, left, rights[i], v);
				}
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}

  private:
	UfomatApp* m_app = nullptr;
};

class FeaturesPanel : public rigkit::IWindow {
  public:
	explicit FeaturesPanel(UfomatApp* app) : IWindow("Features"), m_app(app) {}

	void renderContents() override {
		auto* engine = getEngine();
		auto* ecs = engine ? engine->getECSManager() : nullptr;
		if (!ecs || !m_app) {
			return;
		}
		const auto fea = m_app->ensureFeatures();
		if (fea == entt::null || !ecs->hasComponent<rigkit::ecs::CCode>(fea)) {
			ImGui::TextUnformatted("No features buffer");
			return;
		}
		auto& code = ecs->getComponent<rigkit::ecs::CCode>(fea);
		if (m_buf.capacity() < code.text.size() + 4096) {
			m_buf.resize(code.text.size() + 8192);
		}
		if (m_buf.empty()) {
			m_buf.resize(8192);
		}
		if (code.text.size() >= m_buf.size()) {
			m_buf.resize(code.text.size() + 4096);
		}
		std::snprintf(m_buf.data(), m_buf.size(), "%s", code.text.c_str());
		if (ImGui::InputTextMultiline("##fea", m_buf.data(), m_buf.size(), ImVec2(-1.f, -1.f))) {
			code.text = m_buf.data();
			code.dirty = true;
		}
	}

  private:
	UfomatApp* m_app = nullptr;
	std::vector<char> m_buf;
};

class PreviewPanel : public rigkit::IWindow {
  public:
	explicit PreviewPanel(UfomatApp* app) : IWindow("Preview"), m_app(app) {}

	void renderContents() override {
		auto* engine = getEngine();
		auto* ecs = engine ? engine->getECSManager() : nullptr;
		if (!ecs || !m_app) {
			return;
		}
		char textBuf[256];
		std::snprintf(textBuf, sizeof(textBuf), "%s", m_app->previewText().c_str());
		if (ImGui::InputText("String", textBuf, sizeof(textBuf))) {
			m_app->previewText() = textBuf;
		}
		ImGui::SliderFloat("Size", &m_app->previewSizePx(), 12.f, 240.f, "%.0f");
		ImGui::Checkbox("Kern", &m_app->applyKern());
		for (auto e : ecs->registry().view<rigkit::ecs::CFontAxis>()) {
			const auto& ax = ecs->registry().get<rigkit::ecs::CFontAxis>(e);
			if (ax.tag.empty()) {
				continue;
			}
			if (m_app->location().find(ax.tag) == m_app->location().end()) {
				m_app->location()[ax.tag] = ax.defaultValue;
			}
			ImGui::SliderFloat(ax.tag.c_str(), &m_app->location()[ax.tag], ax.min, ax.max, "%.0f");
		}

		const auto run = rigkit::font::shapeString(*ecs, m_app->previewText(), m_app->applyKern(),
												   m_app->location());
		ImGui::Text("Advance %.0f", static_cast<double>(run.width));

		float upm = 1000.f;
		float desc = -200.f;
		float asc = 800.f;
		for (auto e : ecs->registry().view<rigkit::ecs::CFontFace>()) {
			const auto& f = ecs->registry().get<rigkit::ecs::CFontFace>(e);
			upm = f.unitsPerEm;
			desc = f.descender;
			asc = f.ascender;
			break;
		}
		const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const float height = std::max(48.f, avail.y);
		const float width = std::max(48.f, avail.x);
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x + width, canvasPos.y + height),
						  ImGui::GetColorU32(ImGuiCol_ChildBg));
		const float scale = m_app->previewSizePx() / std::max(1.f, upm);
		const float baseline = canvasPos.y + height * 0.65f;
		const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text, 0.92f);
		auto toScreen = [&](float x, float y) {
			return ImVec2(canvasPos.x + 12.f + x * scale, baseline - y * scale);
		};
		(void)desc;
		(void)asc;
		for (const auto& g : run.glyphs) {
			const auto poly = rigkit::plot::flattenPath(g.path, 2.f);
			for (size_t i = 1; i < poly.size(); ++i) {
				dl->AddLine(toScreen(g.x + poly[i - 1].x, poly[i - 1].y),
							toScreen(g.x + poly[i].x, poly[i].y), col, 1.5f);
			}
		}
		ImGui::Dummy(ImVec2(width, height));
	}

  private:
	UfomatApp* m_app = nullptr;
};

} // namespace

UfomatApp::UfomatApp() {
	window().width = 1100;
	window().height = 720;
	window().title = "UFOMAT";
	settings().appName = "ufomat";
}

void UfomatApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
	if (args.hasFlag("smoke-ttf")) {
		m_smoke = true;
		m_smokeTtf = true;
	}
}

void UfomatApp::registerPacks() {
	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack(std::make_shared<rigkit::rigComponent>());
	packs->registerPack(std::make_shared<rigkit::rigSystems>());
	packs->registerPack(std::make_shared<rigkit::rigProject>());
	packs->registerPack(std::make_shared<rigkit::rigFontComponent>());
	packs->registerPack(std::make_shared<rigkit::rigFont>());
	packs->registerPack(std::make_shared<rigkit::rigUfo>());
	packs->registerPack(std::make_shared<rigkit::rigPlotComponent>());
	packs->registerPack(std::make_shared<rigkit::rigVectorEditor>());
	packs->registerPack(std::make_shared<rigkit::rigImGui>());
	packs->registerPack(std::make_shared<rigkit::rigArtboards>());
	packs->initAll();
	packs->setupAll();
}

void UfomatApp::syncBoardFromFace() {
	auto* ecs = m_engine->getECSManager();
	float upm = 1000.f;
	float asc = 800.f;
	float desc = -200.f;
	if (ecs) {
		for (auto e : ecs->registry().view<rigkit::ecs::CFontFace>()) {
			const auto& f = ecs->registry().get<rigkit::ecs::CFontFace>(e);
			upm = f.unitsPerEm;
			asc = f.ascender;
			desc = f.descender;
			break;
		}
	}
	m_view.yUp = true;
	m_board.id = "em";
	m_board.name = "Em";
	m_board.pos = {0.f, desc};
	m_board.size = {upm, asc - desc};
	m_board.color = {0.96f, 0.96f, 0.94f, 1.f};
	m_board.showGrid = true;
	m_board.gridStepMm = 100.f;
}

void UfomatApp::selectGlyph(entt::entity glyph) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	clearSelection(*ecs);
	m_glyph = glyph;
	m_face = entt::null;
	if (glyph != entt::null) {
		rigkit::ecs::CSelection sel;
		sel.isSelected = true;
		if (ecs->hasComponent<rigkit::ecs::CSelection>(glyph)) {
			ecs->getComponent<rigkit::ecs::CSelection>(glyph) = sel;
		} else {
			ecs->addComponent(glyph, sel);
		}
	}
	syncBoardFromFace();
}

void UfomatApp::selectFirstGlyph() {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	for (auto e : ecs->registry().view<rigkit::ecs::CFontGlyph>()) {
		selectGlyph(e);
		return;
	}
	m_glyph = entt::null;
	m_face = entt::null;
}

void UfomatApp::selectFace(entt::entity face) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	clearSelection(*ecs);
	m_face = face;
	m_glyph = entt::null;
	if (face != entt::null) {
		rigkit::ecs::CSelection sel;
		sel.isSelected = true;
		if (ecs->hasComponent<rigkit::ecs::CSelection>(face)) {
			ecs->getComponent<rigkit::ecs::CSelection>(face) = sel;
		} else {
			ecs->addComponent(face, sel);
		}
	}
	syncBoardFromFace();
}

void UfomatApp::promptOpen() {
	auto* ui = m_engine ? m_engine->getUiManager() : nullptr;
	if (!ui) {
		return;
	}
	ui->openFileDialog("Open UFO", {".ufoz", ".plist"}, [this](const std::string& path) {
		if (!path.empty()) {
			openUfo(path);
		}
	});
}

void UfomatApp::promptExportTtf() {
	auto* ui = m_engine ? m_engine->getUiManager() : nullptr;
	if (!ui) {
		return;
	}
	ui->saveFileDialog("Export TTF", {".ttf"}, [this](const std::string& path) {
		if (path.empty()) {
			return;
		}
		auto* ecs = m_engine->getECSManager();
		if (!ecs) {
			return;
		}
		std::string err;
		if (!rigkit::ufo::exportTtf(*ecs, path, &err)) {
			spdlog::error("UFOMAT export TTF failed: {}", err);
			return;
		}
		spdlog::info("UFOMAT exported {}", path);
	});
}

entt::entity UfomatApp::ensureFeatures() {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return entt::null;
	}
	for (auto e : ecs->registry().view<rigkit::ecs::CCode>()) {
		if (ecs->registry().get<rigkit::ecs::CCode>(e).language == "fea") {
			return e;
		}
	}
	auto fea = ecs->createEntity("features.fea");
	rigkit::ecs::CCode code;
	code.name = "features.fea";
	code.language = "fea";
	ecs->addComponent(fea, code);
	for (auto e : ecs->registry().view<rigkit::ecs::CFontFace>()) {
		ecs->getComponent<rigkit::ecs::CFontFace>(e).features = "features.fea";
		break;
	}
	return fea;
}

void UfomatApp::promptSave() {
	auto* ui = m_engine ? m_engine->getUiManager() : nullptr;
	if (!ui) {
		return;
	}
	ui->saveFileDialog("Save UFO", {".ufo", ".ufoz"}, [this](const std::string& path) {
		if (!path.empty()) {
			saveUfo(path);
		}
	});
}

void UfomatApp::loadContractDemo() {
	auto* ecs = m_engine->getECSManager();
	auto* packs = m_engine->getPackManager();
	if (!ecs || !packs) {
		return;
	}
	auto doc = std::dynamic_pointer_cast<rigkit::rigProject>(packs->getPack("rigProject"));
	if (!doc) {
		return;
	}
	const std::string jsonPath = AppPaths::joinPath(AppPaths::getDataDir(), "font-ufo.json");
	auto imported =
		rigkit::project::importContractFile(*ecs, jsonPath, doc->serializer().registry());
	if (!imported.ok) {
		spdlog::warn("UFOMAT - could not import {}", jsonPath);
		return;
	}
	rigkit::font::rewriteImportedRefs(*ecs, imported.entities);
	selectFirstGlyph();
}

void UfomatApp::openUfo(const std::string& path) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	auto loaded = rigkit::ufo::loadUfo(*ecs, path);
	if (!loaded.ok) {
		spdlog::error("UFOMAT open failed: {}", loaded.error);
		return;
	}
	selectFirstGlyph();
	if (auto* ui = m_engine->getUiManager()) {
		ui->noteRecentFile(path);
	}
	spdlog::info("UFOMAT opened {}", path);
}

void UfomatApp::saveUfo(const std::string& path) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	std::string err;
	if (!rigkit::ufo::saveUfo(*ecs, path, &err)) {
		spdlog::error("UFOMAT save failed: {}", err);
		return;
	}
	if (auto* ui = m_engine->getUiManager()) {
		ui->noteRecentFile(path);
	}
	spdlog::info("UFOMAT saved {}", path);
}

bool UfomatApp::runSmoke() {
	auto* ecs = m_engine->getECSManager();
	auto* packs = m_engine->getPackManager();
	if (!ecs || !packs) {
		return false;
	}
	auto doc = std::dynamic_pointer_cast<rigkit::rigProject>(packs->getPack("rigProject"));
	if (!doc) {
		return false;
	}
	const std::string jsonPath = AppPaths::joinPath(AppPaths::getDataDir(), "font-ufo.json");
	const auto ufoDir = (std::filesystem::temp_directory_path() / "ufomat-smoke.ufo").string();
	std::string err;
	const bool ok = rigkit::ufo::roundtripFromContractJson(*ecs, doc->serializer().registry(),
														   jsonPath, ufoDir, &err);
	if (!ok) {
		spdlog::error("UFOMAT --smoke failed: {}", err);
		return false;
	}
	if (m_smokeTtf && !runTtfSmoke()) {
		return false;
	}
	if (!runVarSmoke()) {
		return false;
	}
	return true;
}

bool UfomatApp::runVarSmoke() {
	auto* ecs = m_engine->getECSManager();
	auto* packs = m_engine->getPackManager();
	if (!ecs || !packs) {
		return false;
	}
	auto doc = std::dynamic_pointer_cast<rigkit::rigProject>(packs->getPack("rigProject"));
	if (!doc) {
		return false;
	}
	const std::string jsonPath = AppPaths::joinPath(AppPaths::getDataDir(), "font-var.json");
	auto imported =
		rigkit::project::importContractFile(*ecs, jsonPath, doc->serializer().registry());
	if (!imported.ok) {
		spdlog::error("UFOMAT --smoke var import failed: {}", imported.error);
		return false;
	}
	rigkit::font::rewriteImportedRefs(*ecs, imported.entities);
	auto glyph = rigkit::font::findNamed(*ecs, "I");
	if (glyph == entt::null) {
		spdlog::error("UFOMAT --smoke var: glyph I missing");
		return false;
	}
	rigkit::ecs::CPath path;
	float adv = 0.f;
	std::unordered_map<std::string, float> loc{{"wght", 900.f}};
	if (!rigkit::font::reconstructGlyph(*ecs, glyph, loc, path, adv) || path.commands.empty()) {
		spdlog::error("UFOMAT --smoke var: reconstruct failed");
		return false;
	}
	if (std::fabs(path.commands[0].p.x - 240.f) > 0.5f) {
		spdlog::error("UFOMAT --smoke var: expected I x=240 got {}", path.commands[0].p.x);
		return false;
	}
	spdlog::info("UFOMAT --smoke var ok");
	return true;
}

bool UfomatApp::runTtfSmoke() {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return false;
	}
	if (!rigkit::ufo::fontmakeAvailable()) {
		spdlog::info("UFOMAT --smoke-ttf skipped (fontmake not on PATH)");
		return true;
	}
	const auto out = (std::filesystem::temp_directory_path() / "ufomat-smoke.ttf").string();
	std::string err;
	if (!rigkit::ufo::exportTtf(*ecs, out, &err)) {
		spdlog::error("UFOMAT --smoke-ttf failed: {}", err);
		return false;
	}
	spdlog::info("UFOMAT --smoke-ttf ok");
	return true;
}

void UfomatApp::setup() {
	spdlog::info("UFOMAT setup");
	m_engine->setClearColor(0.10f, 0.11f, 0.13f, 1.f);
	registerPacks();
	syncBoardFromFace();

	if (m_smoke) {
		m_smokeOk = runSmoke();
		if (m_smokeOk) {
			spdlog::info("UFOMAT --smoke ok");
		}
		if (auto* win = m_engine->getWindow()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
		return;
	}

	loadContractDemo();

	if (auto* mui = dynamic_cast<rigkit::Mui*>(m_engine->getUiManager())) {
		mui->setUndoStack(&m_undo);
		mui->registerFileAction("Open...", [this] { promptOpen(); }, "Ctrl+O");
		mui->registerFileAction("Save...", [this] { promptSave(); }, "Ctrl+S");
		mui->registerExportAction("TTF...", [this] { promptExportTtf(); });
		mui->setRecentFileOpenHandler([this](const std::string& path) {
			if (!path.empty()) {
				openUfo(path);
			}
		});
		if (auto* wm = mui->getWindowManager()) {
			wm->createWindow<GlyphListPanel>(this);
			wm->createWindow<GlyphSurfacePanel>(this);
			wm->createWindow<KernPanel>(this);
			wm->createWindow<FeaturesPanel>(this);
			wm->createWindow<PreviewPanel>(this);
			wm->showWindow("Glyphs");
			wm->showWindow("Glyph");
			wm->showWindow("Properties");
			wm->showWindow("Kern");
			wm->showWindow("Features");
			wm->showWindow("Preview");
		}
		mui->setDockLayoutBuilder([mui](ImGuiID dockspaceId) {
			ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
			if (!node) {
				return;
			}
			if (node->IsSplitNode()) {
				mui->setDockLayoutBuilder(nullptr);
				return;
			}
			const ImGuiViewport* vp = ImGui::GetMainViewport();
			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceId, vp->WorkSize);
			ImGuiID left = 0;
			ImGuiID rest = 0;
			ImGuiID center = 0;
			ImGuiID right = 0;
			ImGuiID bottom = 0;
			ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.18f, &left, &rest);
			ImGui::DockBuilderSplitNode(rest, ImGuiDir_Down, 0.32f, &bottom, &rest);
			ImGui::DockBuilderSplitNode(rest, ImGuiDir_Right, 0.28f, &right, &center);
			ImGui::DockBuilderDockWindow("Glyphs", left);
			ImGui::DockBuilderDockWindow("Glyph", center);
			ImGui::DockBuilderDockWindow("Properties", right);
			ImGui::DockBuilderDockWindow("Kern", bottom);
			ImGui::DockBuilderDockWindow("Features", bottom);
			ImGui::DockBuilderDockWindow("Preview", bottom);
			ImGui::DockBuilderFinish(dockspaceId);
			mui->setDockLayoutBuilder(nullptr);
		});
	}
}
