#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "core/util/MSettings.h"
#include "ecs/MEcs.h"
#include "packs/rigArtboards/src/PathTools.h"
#include "packs/rigArtboards/src/SurfaceChrome.h"
#include "packs/rigArtboards/src/SurfaceSeam.h"
#include "packs/rigArtboards/src/SurfaceView.h"
#include "packs/rigArtboards/src/rigArtboards.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigFont/src/rigFont.h"
#include "packs/rigFontComponent/src/rigFontComponent.h"
#include "packs/rigImGui/src/IWindow.h"
#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/Rulers.h"
#include "packs/rigImGui/src/UiDpi.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigPlotComponent/src/PathBounds.h"
#include "packs/rigPlotComponent/src/PathFlatten.h"
#include "packs/rigPlotComponent/src/rigPlotComponent.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "packs/rigUfo/src/rigUfo.h"
#include "packs/rigVectorEditor/src/PathEditOps.h"
#include "packs/rigVectorEditor/src/rigVectorEditor.h"
#include "CCode.h"
#include "CFontAnchor.h"
#include "CFontAxis.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CFontGroup.h"
#include "CFontKern.h"
#include "CFontLayer.h"
#include "CFontMaster.h"
#include "CName.h"
#include "CPath.h"
#include "CRelationship.h"
#include "CSelection.h"
#include "ContractImport.h"
#include "FontCharsets.h"
#include "FontFind.h"
#include "FontShape.h"
#include "UfoIo.h"
#include "UfoRoundtrip.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

namespace {

ImU32 vec4u32(const glm::vec4& c) {
	const auto byte = [](float v) {
		return static_cast<int>(std::clamp(v, 0.f, 1.f) * 255.f + 0.5f);
	};
	return IM_COL32(byte(c.r), byte(c.g), byte(c.b), byte(c.a));
}

rigkit::artboards::PathChromeStyle chromeFromPrefs(const UfomatSurfacePrefs& p) {
	rigkit::artboards::PathChromeStyle s;
	s.showHandles = p.showHandles;
	s.handlesOnSelectedOnly = p.handlesOnSelectedOnly;
	s.handlePx = std::max(1.f, p.handlePx);
	s.nodePx = std::max(1.f, p.nodePx);
	s.handleShape = p.handleShape;
	s.nodeTypeDisplay = p.nodeTypeDisplay;
	s.handleCol = vec4u32(p.handle);
	s.handleFill = vec4u32(p.handle);
	s.nodeFill = vec4u32(glm::vec4{p.background.r, p.background.g, p.background.b, 1.f});
	s.nodeOutline = vec4u32(p.node);
	s.nodeSelected = vec4u32(p.nodeSelected);
	return s;
}

void clearSelection(rigkit::MEcs& ecs) {
	for (auto e : ecs.registry().view<rigkit::ecs::CSelection>()) {
		ecs.registry().get<rigkit::ecs::CSelection>(e).isSelected = false;
	}
}

/** @brief Push outline + width undo for one glyph entity. */
void pushGlyphSnapshot(rigkit::RigKitEngine* engine, rigkit::IMui* ui, entt::entity entity,
					   const char* label, const rigkit::ecs::CPath& beforePath,
					   const rigkit::ecs::CFontGlyph& beforeGlyph,
					   const rigkit::ecs::CPath& afterPath,
					   const rigkit::ecs::CFontGlyph& afterGlyph) {
	if (!ui || entity == entt::null) {
		return;
	}
	auto* stack = ui->undoStack();
	if (!stack) {
		return;
	}
	struct Snap {
		rigkit::ecs::CPath path;
		rigkit::ecs::CFontGlyph glyph;
	};
	stack->pushSnapshot(label, Snap{beforePath, beforeGlyph}, Snap{afterPath, afterGlyph},
						[engine, entity](const Snap& s) {
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

/** @brief Run a bag-based PathEditOps call on the glyph's single CPath. */
template <typename Fn> bool editAsBag(rigkit::ecs::CPath& path, Fn&& fn) {
	rigkit::ecs::CPaths bag;
	bag.paths.push_back(path);
	if (!fn(bag)) {
		return false;
	}
	path = bag.paths[0];
	return true;
}

/** @brief Index of the tail contour's MoveTo, or -1. */
int tailContourStart(const rigkit::ecs::CPath& path) {
	for (int i = static_cast<int>(path.commands.size()) - 1; i >= 0; --i) {
		if (path.commands[static_cast<size_t>(i)].type == rigkit::ecs::CPath::Cmd::MoveTo) {
			return i;
		}
	}
	return -1;
}

/** @brief True when the tail contour is open (drawable by the pen). */
bool tailContourOpen(const rigkit::ecs::CPath& path) {
	if (path.commands.empty()) {
		return false;
	}
	return path.commands.back().type != rigkit::ecs::CPath::Cmd::Close;
}

/** @brief Outline thumbnail in the em box (asc..desc), centered on the advance. */
void drawGlyphThumb(ImDrawList* dl, const ImVec2& pos, float size, const rigkit::ecs::CPath& path,
					float advance, float upm, float asc, float desc, ImU32 col) {
	const float em = std::max(1.f, asc - desc);
	const float scale = size / std::max(em, upm);
	const float baseline = pos.y + size * (asc / em);
	const float x0 = pos.x + (size - advance * scale) * 0.5f;
	const auto poly = rigkit::plot::flattenPath(path, 6.f);
	for (size_t i = 1; i < poly.size(); ++i) {
		dl->AddLine(ImVec2(x0 + poly[i - 1].x * scale, baseline - poly[i - 1].y * scale),
					ImVec2(x0 + poly[i].x * scale, baseline - poly[i].y * scale), col, 1.f);
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
		ImGui::SameLine();
		if (ImGui::Button(m_grid ? "List" : "Grid")) {
			m_grid = !m_grid;
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
		renderSections(*ecs);
	}

  private:
	/// Group the active layer's glyphs by Unicode block; each section can
	/// generate its missing glyphs into that layer.
	void renderSections(rigkit::MEcs& ecs) {
		const entt::entity layer = m_app->activeLayer();
		const bool filter = layer != entt::null && ecs.registry().valid(layer);
		const auto& sets = rigkit::font::charsets();
		const size_t other = sets.size();
		std::vector<std::vector<entt::entity>> buckets(sets.size() + 1);
		for (auto e : ecs.registry().view<rigkit::ecs::CFontGlyph>()) {
			if (filter && (!ecs.hasComponent<rigkit::ecs::CRelationship>(e) ||
						   ecs.getComponent<rigkit::ecs::CRelationship>(e).parent != layer)) {
				continue;
			}
			const auto& g = ecs.registry().get<rigkit::ecs::CFontGlyph>(e);
			const int idx = g.unicodes.empty() ? -1 : rigkit::font::charsetIndex(g.unicodes[0]);
			buckets[idx < 0 ? other : static_cast<size_t>(idx)].push_back(e);
		}
		auto firstCp = [&ecs](entt::entity e) {
			const auto& u = ecs.registry().get<rigkit::ecs::CFontGlyph>(e).unicodes;
			return u.empty() ? UINT32_MAX : u[0];
		};
		for (auto& bucket : buckets) {
			std::sort(bucket.begin(), bucket.end(),
					  [&](entt::entity a, entt::entity b) { return firstCp(a) < firstCp(b); });
		}
		char label[96];
		for (size_t i = 0; i < sets.size(); ++i) {
			const int total = static_cast<int>(sets[i].last - sets[i].first + 1);
			const int present = static_cast<int>(buckets[i].size());
			std::snprintf(label, sizeof(label), "%s  %d/%d###charset%zu", sets[i].name, present,
						  total, i);
			if (!ImGui::CollapsingHeader(label, i == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
				continue;
			}
			if (present < total) {
				std::snprintf(label, sizeof(label), "Generate missing (%d)###gen%zu",
							  total - present, i);
				if (ImGui::SmallButton(label)) {
					rigkit::font::generateCharset(ecs, sets[i], m_app->activeLayer());
					continue; // buckets are stale this frame
				}
			}
			renderBucket(ecs, buckets[i]);
		}
		if (!buckets[other].empty()) {
			std::snprintf(label, sizeof(label), "Other  %d###charsetOther",
						  static_cast<int>(buckets[other].size()));
			if (ImGui::CollapsingHeader(label)) {
				renderBucket(ecs, buckets[other]);
			}
		}
	}

	void renderBucket(rigkit::MEcs& ecs, const std::vector<entt::entity>& glyphs) {
		if (m_grid) {
			renderGrid(ecs, glyphs);
		} else {
			renderList(ecs, glyphs);
		}
	}

	void renderList(rigkit::MEcs& ecs, const std::vector<entt::entity>& glyphs) {
		for (auto e : glyphs) {
			const auto& g = ecs.registry().get<rigkit::ecs::CFontGlyph>(e);
			std::string label = ecs.entityName(e);
			if (label.empty() && ecs.hasComponent<rigkit::ecs::CName>(e)) {
				label = ecs.getComponent<rigkit::ecs::CName>(e).stableId;
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

	void renderGrid(rigkit::MEcs& ecs, const std::vector<entt::entity>& glyphs) {
		float upm = 1000.f;
		float asc = 800.f;
		float desc = -200.f;
		for (auto e : ecs.registry().view<rigkit::ecs::CFontFace>()) {
			const auto& f = ecs.registry().get<rigkit::ecs::CFontFace>(e);
			upm = f.unitsPerEm;
			asc = f.ascender;
			desc = f.descender;
			break;
		}
		const float cell = rigkit::uiPx(64.f);
		const float labelH = ImGui::GetTextLineHeight();
		const float stride = cell + ImGui::GetStyle().ItemSpacing.x;
		const int cols = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / stride));
		ImDrawList* dl = ImGui::GetWindowDrawList();
		int i = 0;
		for (auto e : glyphs) {
			const auto& g = ecs.registry().get<rigkit::ecs::CFontGlyph>(e);
			std::string label = ecs.entityName(e);
			if (label.empty() && ecs.hasComponent<rigkit::ecs::CName>(e)) {
				label = ecs.getComponent<rigkit::ecs::CName>(e).stableId;
			}
			if (i % cols != 0) {
				ImGui::SameLine();
			}
			++i;
			const bool selected = (e == m_app->selectedGlyph());
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGui::PushID(static_cast<int>(entt::to_integral(e)));
			if (ImGui::Selectable("##cell", selected, 0, ImVec2(cell, cell + labelH))) {
				m_app->selectGlyph(e);
			}
			ImGui::PopID();
			dl->AddRect(pos, ImVec2(pos.x + cell, pos.y + cell),
						ImGui::GetColorU32(ImGuiCol_Border));
			if (ecs.hasComponent<rigkit::ecs::CPath>(e)) {
				drawGlyphThumb(dl, pos, cell, ecs.getComponent<rigkit::ecs::CPath>(e), g.width, upm,
							   asc, desc, ImGui::GetColorU32(ImGuiCol_Text, 0.9f));
			}
			const ImVec2 sz = ImGui::CalcTextSize(label.c_str());
			dl->AddText(ImVec2(pos.x + (cell - sz.x) * 0.5f, pos.y + cell + 1.f),
						ImGui::GetColorU32(ImGuiCol_Text, 0.75f), label.c_str());
		}
	}

	UfomatApp* m_app = nullptr;
	bool m_grid = false;
};

/**
 * @brief Face layers and interpolation masters.
 * @details Layers speak `rig.font.layer`; a master is a layer composing
 * `rig.font.master` with its design-space location on the face's axes.
 */
class LayersPanel : public rigkit::IWindow {
  public:
	explicit LayersPanel(UfomatApp* app) : IWindow("Layers"), m_app(app) {}

	void renderContents() override {
		auto* engine = getEngine();
		auto* ecs = engine ? engine->getECSManager() : nullptr;
		if (!ecs || !m_app) {
			return;
		}
		entt::entity face = m_app->selectedFace();
		if (face == entt::null || !ecs->hasComponent<rigkit::ecs::CFontFace>(face)) {
			face = entt::null;
			for (auto e : ecs->registry().view<rigkit::ecs::CFontFace>()) {
				face = e;
				break;
			}
		}
		if (face == entt::null) {
			ImGui::TextDisabled("No face");
			return;
		}
		renderAxes(*ecs, face);
		ImGui::SeparatorText("Layers");
		const auto layers = rigkit::font::layersOfFace(*ecs, face);
		const entt::entity active = m_app->activeLayer();
		for (auto layer : layers) {
			std::string label = rigkit::font::stableName(*ecs, layer);
			if (label.empty()) {
				label = "layer";
			}
			const auto& lay = ecs->getComponent<rigkit::ecs::CFontLayer>(layer);
			if (lay.role == rigkit::ecs::CFontLayer::Background) {
				label += "  (background)";
			}
			if (ecs->hasComponent<rigkit::ecs::CFontMaster>(layer)) {
				const auto& m = ecs->getComponent<rigkit::ecs::CFontMaster>(layer);
				for (const auto& av : m.location) {
					char loc[48];
					std::snprintf(loc, sizeof(loc), "  %s=%g", av.tag.c_str(), av.value);
					label += loc;
				}
			}
			int glyphCount = 0;
			for (auto g : ecs->registry().view<rigkit::ecs::CFontGlyph>()) {
				if (ecs->hasComponent<rigkit::ecs::CRelationship>(g) &&
					ecs->getComponent<rigkit::ecs::CRelationship>(g).parent == layer) {
					++glyphCount;
				}
			}
			char count[32];
			std::snprintf(count, sizeof(count), "  [%d]", glyphCount);
			label += count;
			ImGui::PushID(static_cast<int>(entt::to_integral(layer)));
			if (ImGui::Selectable(label.c_str(), layer == active)) {
				m_app->selectLayer(layer);
			}
			ImGui::PopID();
		}
		if (ImGui::SmallButton("Add Layer")) {
			addLayer(*ecs, face, false);
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Add Master")) {
			addLayer(*ecs, face, true);
		}
		if (layers.size() > 1 && active != entt::null && ecs->registry().valid(active)) {
			ImGui::SameLine();
			if (ImGui::SmallButton("Delete Layer")) {
				deleteLayer(*ecs, active);
				return;
			}
		}
		renderMaster(*ecs, face, active);
	}

  private:
	std::vector<entt::entity> axesOfFace(rigkit::MEcs& ecs, entt::entity face) {
		std::vector<entt::entity> out;
		for (auto e : ecs.registry().view<rigkit::ecs::CFontAxis>()) {
			if (ecs.hasComponent<rigkit::ecs::CRelationship>(e) &&
				ecs.getComponent<rigkit::ecs::CRelationship>(e).parent == face) {
				out.push_back(e);
			}
		}
		return out;
	}

	void renderAxes(rigkit::MEcs& ecs, entt::entity face) {
		ImGui::SeparatorText("Axes");
		entt::entity kill = entt::null;
		for (auto e : axesOfFace(ecs, face)) {
			auto& ax = ecs.getComponent<rigkit::ecs::CFontAxis>(e);
			ImGui::PushID(static_cast<int>(entt::to_integral(e)));
			char tag[8];
			std::snprintf(tag, sizeof(tag), "%s", ax.tag.c_str());
			ImGui::SetNextItemWidth(rigkit::uiPx(48.f));
			if (ImGui::InputText("##tag", tag, sizeof(tag))) {
				ax.tag = tag;
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(rigkit::uiPx(160.f));
			float mmd[3] = {ax.min, ax.defaultValue, ax.max};
			if (ImGui::InputFloat3("##range", mmd, "%g")) {
				ax.min = mmd[0];
				ax.defaultValue = mmd[1];
				ax.max = mmd[2];
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("X")) {
				kill = e;
			}
			ImGui::PopID();
		}
		if (kill != entt::null) {
			ecs.destroyEntity(kill);
		}
		if (ImGui::SmallButton("Add Axis")) {
			auto e = ecs.createEntity("wght");
			rigkit::ecs::CFontAxis ax;
			ax.tag = "wght";
			ax.name = "Weight";
			ax.min = 100.f;
			ax.defaultValue = 400.f;
			ax.max = 900.f;
			ecs.addComponent(e, ax);
			rigkit::ecs::CName n;
			n.name = "wght";
			n.stableId = "wght";
			ecs.addComponent(e, n);
			rigkit::ecs::CRelationship rel;
			rel.parent = face;
			ecs.addComponent(e, rel);
		}
	}

	void renderMaster(rigkit::MEcs& ecs, entt::entity face, entt::entity layer) {
		if (layer == entt::null || !ecs.registry().valid(layer) ||
			!ecs.hasComponent<rigkit::ecs::CFontLayer>(layer)) {
			return;
		}
		ImGui::SeparatorText("Master");
		if (!ecs.hasComponent<rigkit::ecs::CFontMaster>(layer)) {
			ImGui::TextDisabled("Active layer has no design-space location.");
			if (ImGui::SmallButton("Make Master")) {
				ecs.addComponent(layer, defaultMaster(ecs, face));
			}
			return;
		}
		auto& m = ecs.getComponent<rigkit::ecs::CFontMaster>(layer);
		for (auto e : axesOfFace(ecs, face)) {
			const auto& ax = ecs.getComponent<rigkit::ecs::CFontAxis>(e);
			float v = m.valueFor(ax.tag, ax.defaultValue);
			const char* label = ax.name.empty() ? ax.tag.c_str() : ax.name.c_str();
			ImGui::SetNextItemWidth(rigkit::uiPx(120.f));
			if (ImGui::DragFloat(label, &v, 1.f, ax.min, ax.max, "%g")) {
				m.setValue(ax.tag, v);
			}
		}
		if (ImGui::SmallButton("Remove Master")) {
			ecs.removeComponent<rigkit::ecs::CFontMaster>(layer);
		}
	}

	rigkit::ecs::CFontMaster defaultMaster(rigkit::MEcs& ecs, entt::entity face) {
		rigkit::ecs::CFontMaster m;
		for (auto e : axesOfFace(ecs, face)) {
			const auto& ax = ecs.getComponent<rigkit::ecs::CFontAxis>(e);
			if (!ax.tag.empty()) {
				m.setValue(ax.tag, ax.defaultValue);
			}
		}
		return m;
	}

	/// New master layers copy the active layer's outlines so every master
	/// keeps the same glyph set (interpolation needs matching topology).
	void addLayer(rigkit::MEcs& ecs, entt::entity face, bool master) {
		const auto layers = rigkit::font::layersOfFace(ecs, face);
		char name[32];
		std::snprintf(name, sizeof(name), master ? "master-%zu" : "layer-%zu", layers.size());
		auto e = ecs.createEntity(name);
		rigkit::ecs::CFontLayer lay;
		lay.role = master ? rigkit::ecs::CFontLayer::Spare : rigkit::ecs::CFontLayer::Background;
		ecs.addComponent(e, lay);
		rigkit::ecs::CName n;
		n.name = name;
		n.stableId = name;
		ecs.addComponent(e, n);
		rigkit::ecs::CRelationship rel;
		rel.parent = face;
		ecs.addComponent(e, rel);
		if (master) {
			ecs.addComponent(e, defaultMaster(ecs, face));
			entt::entity source = m_app->activeLayer();
			if (source == entt::null || !ecs.registry().valid(source)) {
				source = layers.empty() ? entt::null : layers.front();
			}
			copyGlyphs(ecs, source, e);
		}
		m_app->selectLayer(e);
	}

	void copyGlyphs(rigkit::MEcs& ecs, entt::entity from, entt::entity to) {
		if (from == entt::null) {
			return;
		}
		std::vector<entt::entity> src;
		for (auto g : ecs.registry().view<rigkit::ecs::CFontGlyph>()) {
			if (ecs.hasComponent<rigkit::ecs::CRelationship>(g) &&
				ecs.getComponent<rigkit::ecs::CRelationship>(g).parent == from) {
				src.push_back(g);
			}
		}
		for (auto g : src) {
			const std::string name = rigkit::font::stableName(ecs, g);
			auto e = ecs.createEntity(name);
			ecs.addComponent(e, ecs.getComponent<rigkit::ecs::CFontGlyph>(g));
			if (ecs.hasComponent<rigkit::ecs::CPath>(g)) {
				ecs.addComponent(e, ecs.getComponent<rigkit::ecs::CPath>(g));
			}
			rigkit::ecs::CName n;
			n.name = name;
			n.stableId = name;
			ecs.addComponent(e, n);
			rigkit::ecs::CRelationship rel;
			rel.parent = to;
			ecs.addComponent(e, rel);
		}
	}

	void deleteLayer(rigkit::MEcs& ecs, entt::entity layer) {
		std::vector<entt::entity> doomed;
		for (auto g : ecs.registry().view<rigkit::ecs::CFontGlyph>()) {
			if (!ecs.hasComponent<rigkit::ecs::CRelationship>(g) ||
				ecs.getComponent<rigkit::ecs::CRelationship>(g).parent != layer) {
				continue;
			}
			for (auto child : ecs.registry().view<rigkit::ecs::CRelationship>()) {
				if (ecs.getComponent<rigkit::ecs::CRelationship>(child).parent == g) {
					doomed.push_back(child);
				}
			}
			doomed.push_back(g);
		}
		doomed.push_back(layer);
		for (auto e : doomed) {
			ecs.destroyEntity(e);
		}
		m_app->selectLayer(entt::null);
	}

	UfomatApp* m_app = nullptr;
};

int currentYear() {
	const std::time_t t = std::time(nullptr);
	const std::tm* tmv = std::localtime(&t);
	return tmv ? tmv->tm_year + 1900 : 2026;
}

std::string firstNonEmpty(const std::string& a, const std::string& b, const std::string& c = {}) {
	if (!a.empty()) {
		return a;
	}
	return b.empty() ? c : b;
}

/** @brief Next version string: empty is 1.000, else bump the third decimal. */
std::string autoVersion(const std::string& cur) {
	const double v = cur.empty() ? 1.0 : std::atof(cur.c_str()) + 0.001;
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%.3f", v);
	return buf;
}

std::string autoCopyright(const rigkit::ecs::CFontFace& f) {
	const std::string owner = firstNonEmpty(f.designer, f.manufacturer, f.family);
	char buf[256];
	std::snprintf(buf, sizeof(buf), "Copyright %d %s. All rights reserved.", currentYear(),
				  owner.c_str());
	return buf;
}

std::string autoTrademark(const rigkit::ecs::CFontFace& f) {
	const std::string owner = firstNonEmpty(f.manufacturer, f.designer, f.family);
	char buf[256];
	std::snprintf(buf, sizeof(buf), "%s is a trademark of %s.",
				  f.family.empty() ? "This font" : f.family.c_str(), owner.c_str());
	return buf;
}

constexpr const char* kOflNotice =
	"This Font Software is licensed under the SIL Open Font License, Version 1.1.";
constexpr const char* kOflUrl = "https://openfontlicense.org";

/** @brief Face metadata editor - fontinfo.plist fields on `CFontFace`. */
class FontInfoPanel : public rigkit::IWindow {
  public:
	explicit FontInfoPanel(UfomatApp* app) : IWindow("Font Info"), m_app(app) {}

	void renderContents() override {
		auto* engine = getEngine();
		auto* ecs = engine ? engine->getECSManager() : nullptr;
		if (!ecs || !m_app) {
			return;
		}
		entt::entity face = m_app->selectedFace();
		if (face == entt::null || !ecs->hasComponent<rigkit::ecs::CFontFace>(face)) {
			for (auto e : ecs->registry().view<rigkit::ecs::CFontFace>()) {
				face = e;
				break;
			}
		}
		if (face == entt::null) {
			ImGui::TextUnformatted("No face");
			return;
		}
		auto& f = ecs->getComponent<rigkit::ecs::CFontFace>(face);

		auto textField = [](const char* label, std::string& value) {
			char buf[256];
			std::snprintf(buf, sizeof(buf), "%s", value.c_str());
			if (ImGui::InputText(label, buf, sizeof(buf))) {
				value = buf;
			}
		};
		auto textBlock = [](const char* label, std::string& value, float lines) {
			char buf[2048];
			std::snprintf(buf, sizeof(buf), "%s", value.c_str());
			const ImVec2 size(-FLT_MIN, ImGui::GetTextLineHeight() * lines +
											ImGui::GetStyle().FramePadding.y * 2.f);
			if (label[0] != '#') {
				ImGui::TextUnformatted(label);
			}
			ImGui::PushID(label);
			if (ImGui::InputTextMultiline("##text", buf, sizeof(buf), size)) {
				value = buf;
			}
			ImGui::PopID();
		};

		// "Auto" fills a field from the others, like the generate buttons in
		// font editors. It overwrites; undo is retyping.
		auto autoButton = [](const char* id) {
			ImGui::SameLine();
			return ImGui::SmallButton(id);
		};

		ImGui::SeparatorText("Identity");
		textField("Family", f.family);
		textField("Style", f.style);
		textField("Version", f.version);
		if (autoButton("Auto##version")) {
			f.version = autoVersion(f.version);
		}

		ImGui::SeparatorText("Dimensions");
		bool metricsChanged = false;
		metricsChanged |= ImGui::InputFloat("Units Per Em", &f.unitsPerEm, 0.f, 0.f, "%.0f");
		metricsChanged |= ImGui::InputFloat("Ascender", &f.ascender, 0.f, 0.f, "%.0f");
		metricsChanged |= ImGui::InputFloat("Descender", &f.descender, 0.f, 0.f, "%.0f");
		metricsChanged |= ImGui::InputFloat("Cap Height", &f.capHeight, 0.f, 0.f, "%.0f");
		metricsChanged |= ImGui::InputFloat("x-Height", &f.xHeight, 0.f, 0.f, "%.0f");
		ImGui::InputFloat("Italic Angle", &f.italicAngle, 0.f, 0.f, "%.1f");
		if (metricsChanged) {
			m_app->syncBoardFromFace();
		}

		ImGui::SeparatorText("Parties");
		textField("Designer", f.designer);
		textField("Designer URL", f.designerUrl);
		textField("Manufacturer", f.manufacturer);
		textField("Manufacturer URL", f.manufacturerUrl);

		ImGui::SeparatorText("Legal");
		ImGui::TextUnformatted("Copyright");
		if (autoButton("Auto##copyright")) {
			f.copyright = autoCopyright(f);
		}
		textBlock("##copyright", f.copyright, 2.f);
		textField("Trademark", f.trademark);
		if (autoButton("Auto##trademark")) {
			f.trademark = autoTrademark(f);
		}
		ImGui::TextUnformatted("Licence");
		if (autoButton("OFL##licence")) {
			f.licence = kOflNotice;
			if (f.licenceUrl.empty()) {
				f.licenceUrl = kOflUrl;
			}
		}
		textBlock("##licence", f.licence, 3.f);
		textField("Licence URL", f.licenceUrl);

		ImGui::SeparatorText("Note");
		textBlock("##note", f.note, 3.f);
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
		auto& g = ecs->getComponent<rigkit::ecs::CFontGlyph>(glyph);
		auto& view = m_app->bedView();
		auto& board = m_app->board();
		ImGuiIO& io = ImGui::GetIO();

		// Face metrics drive the fit, the guides, and metric-line drags.
		float upm = 1000.f;
		float asc = 800.f;
		float desc = -200.f;
		float cap = 700.f;
		float xh = 500.f;
		entt::entity faceEnt = m_app->selectedFace();
		if (faceEnt == entt::null || !ecs->hasComponent<rigkit::ecs::CFontFace>(faceEnt)) {
			faceEnt = entt::null;
			for (auto e : ecs->registry().view<rigkit::ecs::CFontFace>()) {
				faceEnt = e;
				break;
			}
		}
		rigkit::ecs::CFontFace* facePod = nullptr;
		if (faceEnt != entt::null && ecs->hasComponent<rigkit::ecs::CFontFace>(faceEnt)) {
			facePod = &ecs->getComponent<rigkit::ecs::CFontFace>(faceEnt);
			upm = facePod->unitsPerEm;
			asc = facePod->ascender;
			desc = facePod->descender;
			cap = facePod->capHeight;
			xh = facePod->xHeight;
		}

		// Tool rail.
		auto endPen = [&]() {
			if (m_penActive && tailContourOpen(path)) {
				const int tail = tailContourStart(path);
				if (tail >= 0 && tail == static_cast<int>(path.commands.size()) - 1) {
					path.commands.pop_back(); // dangling MoveTo
				}
			}
			m_penActive = false;
			m_penHasOut = false;
			m_penPressed = false;
			m_shapeActive = false;
		};
		auto toolButton = [&](const char* label, Tool t) {
			const bool on = m_tool == t;
			if (on) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
			}
			if (ImGui::SmallButton(label) && m_tool != t) {
				endPen();
				m_tool = t;
			}
			if (on) {
				ImGui::PopStyleColor();
			}
			ImGui::SameLine();
		};
		toolButton("Select (V)", Tool::Select);
		toolButton("Pen (P)", Tool::Pen);
		toolButton("Add (A)", Tool::AddPoint);
		toolButton("Delete (D)", Tool::RemovePoint);
		toolButton("Rect (R)", Tool::Rect);
		toolButton("Ellipse (E)", Tool::Ellipse);
		if (ImGui::SmallButton("Fit (F)")) {
			m_refit = true;
		}

		// Align rail: the picked point's contour, or the whole outline when
		// nothing is picked. Frame is 0..width across, baseline..cap up.
		{
			const float top = cap > 0.f ? cap : (xh > 0.f ? xh : asc);
			int first = 0;
			int last = static_cast<int>(path.commands.size()) - 1;
			if (m_cmd >= 0 && m_cmd <= last) {
				first = m_cmd;
				while (first > 0 && path.commands[static_cast<size_t>(first)].type !=
										rigkit::ecs::CPath::Cmd::MoveTo) {
					--first;
				}
				last = first;
				while (last + 1 < static_cast<int>(path.commands.size()) &&
					   path.commands[static_cast<size_t>(last + 1)].type !=
						   rigkit::ecs::CPath::Cmd::MoveTo) {
					++last;
				}
			}
			rigkit::plot::PathBounds b;
			if (!path.commands.empty() && last >= first) {
				rigkit::ecs::CPath part;
				part.commands.assign(path.commands.begin() + first,
									 path.commands.begin() + last + 1);
				b = rigkit::plot::boundsOfPath(part, 1.f);
			}
			auto alignTo = [&](float dx, float dy, const char* label) {
				if (std::fabs(dx) < 1e-4f && std::fabs(dy) < 1e-4f) {
					return;
				}
				m_snapPath = path;
				m_snapGlyph = g;
				const glm::vec2 d{std::round(dx), std::round(dy)};
				for (int i = first; i <= last; ++i) {
					auto& c = path.commands[static_cast<size_t>(i)];
					c.p += d;
					c.c1 += d;
					c.c2 += d;
				}
				pushGlyphSnapshot(engine, ui, glyph, label, m_snapPath, m_snapGlyph, path, g);
			};
			ImGui::TextDisabled(m_cmd >= 0 ? "Align contour" : "Align all");
			ImGui::SameLine();
			ImGui::BeginDisabled(b.empty);
			if (ImGui::SmallButton("Left")) {
				alignTo(-b.minX, 0.f, "Align left");
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Center")) {
				alignTo(g.width * 0.5f - (b.minX + b.maxX) * 0.5f, 0.f, "Align center");
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Right")) {
				alignTo(g.width - b.maxX, 0.f, "Align right");
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Base")) {
				alignTo(0.f, -b.minY, "Align baseline");
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Middle")) {
				alignTo(0.f, top * 0.5f - (b.minY + b.maxY) * 0.5f, "Align middle");
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Cap")) {
				alignTo(0.f, top - b.maxY, "Align cap");
			}
			ImGui::EndDisabled();
		}
		if (ImGui::IsWindowFocused() && !io.WantTextInput) {
			if (ImGui::IsKeyPressed(ImGuiKey_V)) {
				endPen();
				m_tool = Tool::Select;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_P)) {
				m_tool = Tool::Pen;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_A)) {
				endPen();
				m_tool = Tool::AddPoint;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_D)) {
				endPen();
				m_tool = Tool::RemovePoint;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_R)) {
				endPen();
				m_tool = Tool::Rect;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_E)) {
				endPen();
				m_tool = Tool::Ellipse;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_F)) {
				m_refit = true;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
				endPen();
			}
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const float width = std::max(rigkit::uiPx(80.f), avail.x);
		const float height = std::max(rigkit::uiPx(80.f), avail.y);
		const float uiScale = rigkit::uiPx(1.f);
		if (glyph != m_lastGlyph) {
			m_lastGlyph = glyph;
			m_refit = true;
			endPen();
			m_cmd = -1;
		}
		const bool resized = std::fabs(width - m_fitW) > 1.f || std::fabs(height - m_fitH) > 1.f;
		if ((m_refit || resized) && width > rigkit::uiPx(120.f) && height > rigkit::uiPx(120.f)) {
			m_surface.fit(view, ImVec2(width, height), {0.f, desc, std::max(upm, g.width), asc},
						  rigkit::rulerStripThickness(uiScale) + rigkit::uiPx(12.f));
			m_fitW = width;
			m_fitH = height;
			m_refit = false;
		}

		ImGui::InvisibleButton("glyph_canvas", rigkit::uiHitSize(ImVec2(width, height)),
							   ImGuiButtonFlags_MouseButtonLeft |
								   ImGuiButtonFlags_MouseButtonMiddle);
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();
		const bool middle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

		rigkit::artboards::SurfaceMap map;
		map.canvasPos = canvasPos;
		map.view = &view;
		const glm::vec2 mouse = map.toMm(io.MousePos);
		const glm::vec2 localPx{io.MousePos.x - canvasPos.x, io.MousePos.y - canvasPos.y};
		auto toScreen = [&](const glm::vec2& mm) {
			const glm::vec2 px = view.mmToPx(mm);
			return ImVec2(canvasPos.x + px.x, canvasPos.y + px.y);
		};
		// Drawing tools snap to whole font units.
		const glm::vec2 snapped{std::round(mouse.x), std::round(mouse.y)};

		if (hovered && io.MouseWheel != 0.f) {
			m_surface.handleWheel(view, localPx, io.MouseWheel);
		}
		if (active && middle) {
			m_surface.handlePan(view, {io.MouseDelta.x, io.MouseDelta.y});
		}

		const float tol = std::max(4.f, 8.f / std::max(0.05f, view.scalePxPerMm));
		const float hitPx = rigkit::uiPx(4.f);
		auto metricUnderMouse = [&]() {
			if (facePod) {
				if (std::fabs(io.MousePos.y - toScreen({0.f, asc}).y) < hitPx) {
					return MetricDrag::Asc;
				}
				if (std::fabs(io.MousePos.y - toScreen({0.f, cap}).y) < hitPx) {
					return MetricDrag::Cap;
				}
				if (std::fabs(io.MousePos.y - toScreen({0.f, xh}).y) < hitPx) {
					return MetricDrag::XH;
				}
				if (std::fabs(io.MousePos.y - toScreen({0.f, desc}).y) < hitPx) {
					return MetricDrag::Desc;
				}
			}
			if (std::fabs(io.MousePos.x - toScreen({g.width, 0.f}).x) < hitPx) {
				return MetricDrag::Width;
			}
			return MetricDrag::None;
		};

		if (m_tool == Tool::Select) {
			if (hovered && !middle && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				int cmd = -1;
				rigkit::ecs::PathEditPart part = rigkit::ecs::PathEditPart::None;
				if (rigkit::pathEdit::pickHit(path, mouse, tol, cmd, part) &&
					part == rigkit::ecs::PathEditPart::Endpoint) {
					m_snapPath = path;
					m_snapGlyph = g;
					m_cmd = cmd;
					m_part = part;
					if (rigkit::pathEdit::toggleEndpointSmooth(path, cmd)) {
						pushGlyphSnapshot(engine, ui, glyph, "Toggle node type", m_snapPath,
										  m_snapGlyph, path, g);
					}
				}
				m_dragging = false;
				m_haveSnap = false;
			} else if (hovered && !middle && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				m_snapPath = path;
				m_snapGlyph = g;
				m_haveSnap = true;
				m_dragMetric = MetricDrag::None;
				if (rigkit::pathEdit::pickHit(path, mouse, tol, m_cmd, m_part)) {
					m_lastMouse = mouse;
					m_dragging = true;
				} else {
					m_cmd = -1;
					m_part = rigkit::ecs::PathEditPart::None;
					m_dragging = false;
					m_dragMetric = metricUnderMouse();
				}
			}
			if (active && m_dragging && !middle && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				const glm::vec2 delta = mouse - m_lastMouse;
				rigkit::pathEdit::moveHit(path, m_cmd, m_part, delta, io.KeyAlt, io.KeyShift);
				m_lastMouse = mouse;
			}
			if (active && m_dragMetric != MetricDrag::None && !middle &&
				ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				const float yu = std::round(mouse.y);
				switch (m_dragMetric) {
				case MetricDrag::Asc:
					facePod->ascender = std::max(1.f, yu);
					break;
				case MetricDrag::Cap:
					facePod->capHeight = yu;
					break;
				case MetricDrag::XH:
					facePod->xHeight = yu;
					break;
				case MetricDrag::Desc:
					facePod->descender = std::min(0.f, yu);
					break;
				case MetricDrag::Width:
					g.width = std::max(0.f, std::round(mouse.x));
					break;
				default:
					break;
				}
				if (facePod && m_dragMetric != MetricDrag::Width) {
					m_app->syncBoardFromFace();
				}
			}
			// Cursor feedback on metric lines.
			if (hovered && !m_dragging) {
				const MetricDrag target = (active && m_dragMetric != MetricDrag::None)
											  ? m_dragMetric
											  : metricUnderMouse();
				if (target == MetricDrag::Width) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
				} else if (target != MetricDrag::None) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
				}
			}
			if (ImGui::IsItemDeactivated()) {
				if (m_haveSnap && (m_dragging || m_dragMetric == MetricDrag::Width)) {
					pushGlyphSnapshot(engine, ui, glyph,
									  m_dragMetric == MetricDrag::Width ? "Set width"
																		: "Edit outline",
									  m_snapPath, m_snapGlyph, path, g);
				}
				m_dragging = false;
				m_haveSnap = false;
				m_dragMetric = MetricDrag::None;
			}
		} else if (m_tool == Tool::Pen) {
			if (hovered && !middle && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				m_snapPath = path;
				m_snapGlyph = g;
				m_penDown = snapped;
				m_penPressed = true;
				if (!m_penActive || !tailContourOpen(path)) {
					path.commands.push_back({rigkit::ecs::CPath::Cmd::MoveTo, snapped, {}, {}});
					m_penActive = true;
					m_penFresh = true;
					m_penHasOut = false;
				} else {
					m_penFresh = false;
				}
			}
			if (m_penPressed && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				m_penPressed = false;
				const bool draggedOut = glm::length(snapped - m_penDown) > tol;
				const int tail = tailContourStart(path);
				const char* label = "Pen point";
				if (m_penFresh) {
					// First anchor of the contour; a drag sets its outgoing handle.
					m_penHasOut = draggedOut;
					if (draggedOut) {
						m_penOut = snapped;
					}
					label = "Pen start";
				} else if (tail >= 0 &&
						   glm::length(m_penDown - path.commands[static_cast<size_t>(tail)].p) <=
							   tol &&
						   static_cast<int>(path.commands.size()) - tail > 1) {
					path.commands.push_back({rigkit::ecs::CPath::Cmd::Close, {}, {}, {}});
					m_penActive = false;
					m_penHasOut = false;
					label = "Pen close";
				} else if (draggedOut) {
					// Drag shapes the curve: c1 = previous outgoing, c2 mirrors the drag.
					const glm::vec2 c1 = m_penHasOut ? m_penOut : path.commands.back().p;
					const glm::vec2 c2 = m_penDown * 2.f - snapped;
					path.commands.push_back({rigkit::ecs::CPath::Cmd::CubicTo, m_penDown, c1, c2});
					m_penOut = snapped;
					m_penHasOut = true;
					label = "Pen curve";
				} else if (m_penHasOut) {
					path.commands.push_back(
						{rigkit::ecs::CPath::Cmd::CubicTo, m_penDown, m_penOut, m_penDown});
					m_penHasOut = false;
				} else {
					path.commands.push_back({rigkit::ecs::CPath::Cmd::LineTo, m_penDown, {}, {}});
				}
				pushGlyphSnapshot(engine, ui, glyph, label, m_snapPath, m_snapGlyph, path, g);
			}
		} else if (m_tool == Tool::AddPoint) {
			if (hovered && !middle && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				int cmd = -1;
				rigkit::ecs::PathEditPart part = rigkit::ecs::PathEditPart::None;
				if (rigkit::pathEdit::pickHit(path, mouse, tol, cmd, part) &&
					part == rigkit::ecs::PathEditPart::Segment) {
					m_snapPath = path;
					m_snapGlyph = g;
					const float t = rigkit::artboards::segmentHitT(path, cmd, mouse);
					if (editAsBag(path, [&](rigkit::ecs::CPaths& bag) {
							return rigkit::pathEdit::insertOnSegment(bag, 0, cmd, t) >= 0;
						})) {
						pushGlyphSnapshot(engine, ui, glyph, "Add point", m_snapPath, m_snapGlyph,
										  path, g);
					}
				}
			}
		} else if (m_tool == Tool::RemovePoint) {
			if (hovered && !middle && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				int cmd = -1;
				rigkit::ecs::PathEditPart part = rigkit::ecs::PathEditPart::None;
				if (rigkit::pathEdit::pickHit(path, mouse, tol, cmd, part) &&
					part == rigkit::ecs::PathEditPart::Endpoint) {
					m_snapPath = path;
					m_snapGlyph = g;
					if (editAsBag(path, [&](rigkit::ecs::CPaths& bag) {
							return rigkit::pathEdit::deleteCommand(bag, 0, cmd);
						})) {
						m_cmd = -1;
						pushGlyphSnapshot(engine, ui, glyph, "Delete point", m_snapPath,
										  m_snapGlyph, path, g);
					}
				}
			}
		} else if (m_tool == Tool::Rect || m_tool == Tool::Ellipse) {
			if (hovered && !middle && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				m_snapPath = path;
				m_snapGlyph = g;
				m_shape0 = snapped;
				m_shapeActive = true;
			}
		}

		const auto& prefs = m_app->surfacePrefs();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 canvasMax(canvasPos.x + width, canvasPos.y + height);
		dl->AddRectFilled(canvasPos, canvasMax, vec4u32(prefs.background), style.FrameRounding);
		dl->AddRect(canvasPos, canvasMax, ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding);
		dl->PushClipRect(canvasPos, canvasMax, true);

		const std::vector<const rigkit::ecs::CArtboard*> boards{&board};
		m_surface.drawArtboards(dl, canvasPos, view, boards, false);
		if (board.showGrid) {
			auto grid = prefs.metrics;
			grid.a *= 0.45f;
			m_surface.drawGrid(
				dl, canvasPos, view,
				{board.pos.x, board.pos.y, board.pos.x + board.size.x, board.pos.y + board.size.y},
				board.gridStepMm, vec4u32(grid));
		}

		const ImU32 guide = vec4u32(prefs.metrics);
		const ImU32 labelCol = vec4u32(prefs.labels);
		const float rulerPx = rigkit::rulerStripThickness(uiScale);
		// Metric lines run the full canvas so the window reads as one surface.
		auto hline = [&](float y, ImU32 col, float th) {
			const float sy = toScreen({0.f, y}).y;
			dl->AddLine(ImVec2(canvasPos.x, sy), ImVec2(canvasMax.x, sy), col, th);
		};
		auto vline = [&](float x, float th) {
			const float sx = toScreen({x, 0.f}).x;
			dl->AddLine(ImVec2(sx, canvasPos.y), ImVec2(sx, canvasMax.y), guide, th);
		};
		hline(0.f, guide, 1.5f);
		hline(asc, guide, 1.f);
		hline(desc, guide, 1.f);
		hline(cap, guide, 1.f);
		hline(xh, guide, 1.f);
		vline(0.f, 1.f);
		vline(g.width, 1.5f);

		if (prefs.showMetricLabels) {
			auto metricLabel = [&](float y, const char* name) {
				const float sy = toScreen({0.f, y}).y;
				const ImVec2 sz = ImGui::CalcTextSize(name);
				dl->AddText(ImVec2(canvasPos.x + rulerPx + 6.f, sy - sz.y - 2.f), labelCol, name);
				if (prefs.showMetricValues) {
					char buf[32];
					std::snprintf(buf, sizeof(buf), "%.0f", y);
					const ImVec2 vz = ImGui::CalcTextSize(buf);
					dl->AddText(ImVec2(canvasMax.x - vz.x - 6.f, sy - vz.y - 2.f), labelCol, buf);
				}
			};
			metricLabel(asc, "Ascender");
			metricLabel(cap, "Cap Height");
			metricLabel(xh, "x-Height");
			metricLabel(0.f, "Baseline");
			if (std::fabs(desc) > 0.5f) {
				metricLabel(desc, "Descender");
			}
			char wbuf[48];
			std::snprintf(wbuf, sizeof(wbuf), "%.0f", g.width);
			const ImVec2 wsz = ImGui::CalcTextSize(wbuf);
			dl->AddText(ImVec2(toScreen({g.width, 0.f}).x - wsz.x - 4.f, canvasMax.y - wsz.y - 4.f),
						labelCol, wbuf);
		}

		// Ghost the same glyph from the face's other layers (masters,
		// background) so masters can be drawn against each other.
		if (prefs.showOtherLayers && faceEnt != entt::null) {
			const std::string glyphName = rigkit::font::stableName(*ecs, glyph);
			const entt::entity ownLayer =
				ecs->hasComponent<rigkit::ecs::CRelationship>(glyph)
					? ecs->getComponent<rigkit::ecs::CRelationship>(glyph).parent
					: entt::null;
			const ImU32 ghostCol = vec4u32(prefs.ghost);
			for (auto layer : rigkit::font::layersOfFace(*ecs, faceEnt)) {
				if (layer == ownLayer) {
					continue;
				}
				const auto other = rigkit::font::findGlyphInLayer(*ecs, layer, glyphName);
				if (other == entt::null || !ecs->hasComponent<rigkit::ecs::CPath>(other)) {
					continue;
				}
				const auto gp =
					rigkit::plot::flattenPath(ecs->getComponent<rigkit::ecs::CPath>(other), 2.f);
				for (size_t i = 1; i < gp.size(); ++i) {
					dl->AddLine(toScreen(gp[i - 1]), toScreen(gp[i]), ghostCol, rigkit::uiPx(1.f));
				}
			}
		}

		const auto poly = rigkit::plot::flattenPath(path, 2.f);
		const ImU32 pathCol = vec4u32(prefs.path);
		for (size_t i = 1; i < poly.size(); ++i) {
			dl->AddLine(toScreen(poly[i - 1]), toScreen(poly[i]), pathCol, rigkit::uiPx(2.f));
		}
		rigkit::artboards::drawPathAnchors(dl, map, path, m_cmd, chromeFromPrefs(prefs));

		if (prefs.showUfoAnchors) {
			const ImU32 ancCol = vec4u32(prefs.node);
			const float r = std::max(3.f, prefs.nodePx + 1.f);
			for (auto e : ecs->registry().view<rigkit::ecs::CFontAnchor>()) {
				if (!ecs->hasComponent<rigkit::ecs::CRelationship>(e)) {
					continue;
				}
				if (ecs->getComponent<rigkit::ecs::CRelationship>(e).parent != glyph) {
					continue;
				}
				const auto& a = ecs->getComponent<rigkit::ecs::CFontAnchor>(e);
				const ImVec2 p = toScreen(a.point);
				dl->AddLine(ImVec2(p.x - r, p.y), ImVec2(p.x + r, p.y), ancCol, 1.5f);
				dl->AddLine(ImVec2(p.x, p.y - r), ImVec2(p.x, p.y + r), ancCol, 1.5f);
				dl->AddLine(ImVec2(p.x - r * 0.7f, p.y - r * 0.7f),
							ImVec2(p.x + r * 0.7f, p.y + r * 0.7f), ancCol, 1.f);
				dl->AddLine(ImVec2(p.x - r * 0.7f, p.y + r * 0.7f),
							ImVec2(p.x + r * 0.7f, p.y - r * 0.7f), ancCol, 1.f);
				if (ecs->hasComponent<rigkit::ecs::CName>(e)) {
					const auto& n = ecs->getComponent<rigkit::ecs::CName>(e);
					if (!n.stableId.empty()) {
						dl->AddText(ImVec2(p.x + r + 4.f, p.y - 8.f), labelCol, n.stableId.c_str());
					}
				}
			}
		}

		// Pen rubber band + snapped target.
		if (m_tool == Tool::Pen && hovered && !middle) {
			const ImU32 penCol = vec4u32(prefs.handle);
			if (m_penActive && tailContourOpen(path) && !path.commands.empty()) {
				dl->AddLine(toScreen(path.commands.back().p), toScreen(snapped), penCol, 1.f);
			}
			if (m_penPressed && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
				glm::length(snapped - m_penDown) > tol) {
				const ImVec2 a = toScreen(m_penDown);
				dl->AddLine(toScreen(m_penDown * 2.f - snapped), a, penCol, 1.f);
				dl->AddLine(a, toScreen(snapped), penCol, 1.f);
			}
			dl->AddCircle(toScreen(snapped), rigkit::uiPx(3.f), penCol, 0, 1.5f);
		}

		// Shape rubber band: preview while dragging, commit a new contour on
		// release. Shift constrains to a square / circle.
		if ((m_tool == Tool::Rect || m_tool == Tool::Ellipse) && m_shapeActive) {
			glm::vec2 b = snapped;
			if (io.KeyShift) {
				const glm::vec2 d = b - m_shape0;
				const float ext = std::max(std::abs(d.x), std::abs(d.y));
				b = m_shape0 + glm::vec2{d.x < 0.f ? -ext : ext, d.y < 0.f ? -ext : ext};
			}
			const glm::vec2 mn = glm::min(m_shape0, b);
			const glm::vec2 mx = glm::max(m_shape0, b);
			const bool isRect = m_tool == Tool::Rect;
			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
				m_shapeActive = false;
			} else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				const ImU32 col = vec4u32(prefs.handle);
				const ImVec2 pa = toScreen(mn);
				const ImVec2 pb = toScreen(mx);
				const ImVec2 p0(std::min(pa.x, pb.x), std::min(pa.y, pb.y));
				const ImVec2 p1(std::max(pa.x, pb.x), std::max(pa.y, pb.y));
				if (isRect) {
					dl->AddRect(p0, p1, col, 0.f, 0, 1.5f);
				} else {
					dl->AddEllipse(ImVec2((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f),
								   ImVec2((p1.x - p0.x) * 0.5f, (p1.y - p0.y) * 0.5f), col, 0.f, 0,
								   1.5f);
				}
			} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				m_shapeActive = false;
				if (mx.x - mn.x >= 1.f && mx.y - mn.y >= 1.f) {
					using Cmd = rigkit::ecs::CPath::Cmd;
					if (isRect) {
						// Counter-clockwise in y-up font units = filled outer contour.
						path.commands.push_back({Cmd::MoveTo, mn, {}, {}});
						path.commands.push_back({Cmd::LineTo, {mx.x, mn.y}, {}, {}});
						path.commands.push_back({Cmd::LineTo, mx, {}, {}});
						path.commands.push_back({Cmd::LineTo, {mn.x, mx.y}, {}, {}});
						path.commands.push_back({Cmd::Close, {}, {}, {}});
					} else {
						const glm::vec2 c = (mn + mx) * 0.5f;
						const float rx = (mx.x - mn.x) * 0.5f;
						const float ry = (mx.y - mn.y) * 0.5f;
						constexpr float k = 0.5522847498f; // cubic kappa
						const float kx = rx * k;
						const float ky = ry * k;
						path.commands.push_back({Cmd::MoveTo, {c.x + rx, c.y}, {}, {}});
						path.commands.push_back({Cmd::CubicTo,
												 {c.x, c.y + ry},
												 {c.x + rx, c.y + ky},
												 {c.x + kx, c.y + ry}});
						path.commands.push_back({Cmd::CubicTo,
												 {c.x - rx, c.y},
												 {c.x - kx, c.y + ry},
												 {c.x - rx, c.y + ky}});
						path.commands.push_back({Cmd::CubicTo,
												 {c.x, c.y - ry},
												 {c.x - rx, c.y - ky},
												 {c.x - kx, c.y - ry}});
						path.commands.push_back({Cmd::CubicTo,
												 {c.x + rx, c.y},
												 {c.x + kx, c.y - ry},
												 {c.x + rx, c.y - ky}});
						path.commands.push_back({Cmd::Close, {}, {}, {}});
					}
					pushGlyphSnapshot(engine, ui, glyph, isRect ? "Rect" : "Ellipse", m_snapPath,
									  m_snapGlyph, path, g);
				}
			}
		}

		rigkit::drawRulersInRegion(dl, canvasPos, ImVec2(width, height), io.MousePos,
								   view.scalePxPerMm, "u", uiScale, toScreen({0.f, 0.f}), true);
		dl->PopClipRect();
	}

  private:
	enum class Tool : int { Select = 0, Pen, AddPoint, RemovePoint, Rect, Ellipse };
	enum class MetricDrag : int { None = 0, Asc, Cap, XH, Desc, Width };

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
	bool m_refit = true;
	entt::entity m_lastGlyph = entt::null;
	Tool m_tool = Tool::Select;
	MetricDrag m_dragMetric = MetricDrag::None;
	bool m_penActive = false;
	bool m_penFresh = false;
	bool m_penHasOut = false;
	bool m_penPressed = false;
	glm::vec2 m_penDown{0.f};
	glm::vec2 m_penOut{0.f};
	bool m_shapeActive = false;
	glm::vec2 m_shape0{0.f};
};

std::string glyphLabel(rigkit::MEcs& ecs, entt::entity e) {
	std::string label = rigkit::font::stableName(ecs, e);
	if (label.empty()) {
		label = ecs.entityName(e);
	}
	return label;
}

std::string utf8FromCp(uint32_t cp) {
	char buf[5]{};
	if (cp < 0x80) {
		buf[0] = static_cast<char>(cp);
	} else if (cp < 0x800) {
		buf[0] = static_cast<char>(0xC0 | (cp >> 6));
		buf[1] = static_cast<char>(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		buf[0] = static_cast<char>(0xE0 | (cp >> 12));
		buf[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		buf[2] = static_cast<char>(0x80 | (cp & 0x3F));
	} else {
		buf[0] = static_cast<char>(0xF0 | (cp >> 18));
		buf[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
		buf[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		buf[3] = static_cast<char>(0x80 | (cp & 0x3F));
	}
	return buf;
}

std::string glyphPreviewChar(rigkit::MEcs& ecs, const std::string& name) {
	auto e = rigkit::font::findNamed(ecs, name);
	if (e != entt::null && ecs.hasComponent<rigkit::ecs::CFontGlyph>(e)) {
		const auto& g = ecs.getComponent<rigkit::ecs::CFontGlyph>(e);
		if (!g.unicodes.empty()) {
			return utf8FromCp(g.unicodes[0]);
		}
	}
	if (e != entt::null && ecs.hasComponent<rigkit::ecs::CFontGroup>(e)) {
		const auto& group = ecs.getComponent<rigkit::ecs::CFontGroup>(e);
		if (!group.members.empty()) {
			return glyphPreviewChar(ecs, group.members.front());
		}
	}
	return name.size() == 1 ? name : std::string{};
}

std::string shortGroupName(const std::string& name) {
	constexpr const char* k1 = "public.kern1.";
	constexpr const char* k2 = "public.kern2.";
	if (name.rfind(k1, 0) == 0) {
		return name.substr(std::strlen(k1));
	}
	if (name.rfind(k2, 0) == 0) {
		return name.substr(std::strlen(k2));
	}
	return name;
}

void drawShapedPreview(rigkit::MEcs& ecs, const rigkit::font::ShapedRun& run, float sizePx,
					   float heightPx = 0.f) {
	float upm = 1000.f;
	for (auto e : ecs.registry().view<rigkit::ecs::CFontFace>()) {
		upm = ecs.registry().get<rigkit::ecs::CFontFace>(e).unitsPerEm;
		break;
	}
	const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float height = heightPx > 0.f ? heightPx : std::max(48.f, avail.y);
	const float width = std::max(48.f, avail.x);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x + width, canvasPos.y + height),
					  ImGui::GetColorU32(ImGuiCol_ChildBg));
	const float scale = sizePx / std::max(1.f, upm);
	const float baseline = canvasPos.y + height * 0.7f;
	const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text, 0.92f);
	for (const auto& g : run.glyphs) {
		const auto poly = rigkit::plot::flattenPath(g.path, 2.f);
		for (size_t i = 1; i < poly.size(); ++i) {
			const ImVec2 a{canvasPos.x + 12.f + (g.x + poly[i - 1].x) * scale,
						   baseline - poly[i - 1].y * scale};
			const ImVec2 b{canvasPos.x + 12.f + (g.x + poly[i].x) * scale,
						   baseline - poly[i].y * scale};
			dl->AddLine(a, b, col, 1.5f);
		}
	}
	ImGui::Dummy(ImVec2(width, height));
}

bool pathXBounds(const rigkit::ecs::CPath& path, float& minX, float& maxX) {
	const auto poly = rigkit::plot::flattenPath(path, 2.f);
	if (poly.empty()) {
		return false;
	}
	minX = maxX = poly[0].x;
	for (const auto& p : poly) {
		minX = std::min(minX, p.x);
		maxX = std::max(maxX, p.x);
	}
	return true;
}

void drawKernMetricsTable(rigkit::MEcs& ecs, const rigkit::font::ShapedRun& run,
						  std::string* selectLeft = nullptr, std::string* selectRight = nullptr) {
	if (run.glyphs.empty()) {
		return;
	}
	const int n = static_cast<int>(run.glyphs.size());
	if (!ImGui::BeginTable("kern_metrics", n + 1,
						   ImGuiTableFlags_ScrollX | ImGuiTableFlags_Borders |
							   ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
		return;
	}
	ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 72.f);
	for (int i = 0; i < n; ++i) {
		ImGui::TableSetupColumn(run.glyphs[static_cast<size_t>(i)].name.c_str(),
								ImGuiTableColumnFlags_WidthFixed, 64.f);
	}

	auto labelRow = [](const char* title) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(title);
	};

	labelRow("");
	for (int i = 0; i < n; ++i) {
		ImGui::TableSetColumnIndex(i + 1);
		const auto& g = run.glyphs[static_cast<size_t>(i)];
		ImGui::PushID(i);
		if (ImGui::Selectable(g.name.c_str(), false)) {
			if (selectLeft) {
				*selectLeft = g.name;
			}
			if (selectRight && i + 1 < n) {
				*selectRight = run.glyphs[static_cast<size_t>(i + 1)].name;
			}
		}
		ImGui::PopID();
	}

	// Width / sidebearing edits write the source glyph (name lookup), not the
	// shaped copy. LSB translates the outline and keeps RSB by growing width;
	// RSB only moves the advance.
	auto sourceGlyph = [&](const std::string& name) -> rigkit::ecs::CFontGlyph* {
		const auto e = rigkit::font::findNamed(ecs, name);
		if (e == entt::null || !ecs.hasComponent<rigkit::ecs::CFontGlyph>(e)) {
			return nullptr;
		}
		return &ecs.getComponent<rigkit::ecs::CFontGlyph>(e);
	};
	auto sourcePath = [&](const std::string& name) -> rigkit::ecs::CPath* {
		const auto e = rigkit::font::findNamed(ecs, name);
		if (e == entt::null || !ecs.hasComponent<rigkit::ecs::CPath>(e)) {
			return nullptr;
		}
		return &ecs.getComponent<rigkit::ecs::CPath>(e);
	};

	labelRow("Width");
	for (int i = 0; i < n; ++i) {
		ImGui::TableSetColumnIndex(i + 1);
		const auto& g = run.glyphs[static_cast<size_t>(i)];
		float w = g.advance;
		ImGui::PushID(2000 + i);
		ImGui::SetNextItemWidth(-1.f);
		if (ImGui::InputFloat("##w", &w, 0.f, 0.f, "%.0f")) {
			if (auto* src = sourceGlyph(g.name)) {
				src->width = w;
			}
		}
		ImGui::PopID();
	}

	labelRow("L Side");
	for (int i = 0; i < n; ++i) {
		ImGui::TableSetColumnIndex(i + 1);
		const auto& g = run.glyphs[static_cast<size_t>(i)];
		float minX = 0.f;
		float maxX = 0.f;
		const bool haveBounds = pathXBounds(g.path, minX, maxX);
		float lsb = haveBounds ? minX : 0.f;
		ImGui::PushID(3000 + i);
		ImGui::SetNextItemWidth(-1.f);
		if (ImGui::InputFloat("##lsb", &lsb, 0.f, 0.f, "%.0f") && haveBounds) {
			const float delta = lsb - minX;
			auto* path = sourcePath(g.name);
			auto* src = sourceGlyph(g.name);
			if (path && src && delta != 0.f) {
				rigkit::pathEdit::translatePath(*path, {delta, 0.f});
				src->width += delta;
			}
		}
		ImGui::PopID();
	}

	labelRow("R Side");
	for (int i = 0; i < n; ++i) {
		ImGui::TableSetColumnIndex(i + 1);
		const auto& g = run.glyphs[static_cast<size_t>(i)];
		float minX = 0.f;
		float maxX = 0.f;
		const bool haveBounds = pathXBounds(g.path, minX, maxX);
		float rsb = haveBounds ? g.advance - maxX : g.advance;
		ImGui::PushID(4000 + i);
		ImGui::SetNextItemWidth(-1.f);
		if (ImGui::InputFloat("##rsb", &rsb, 0.f, 0.f, "%.0f") && haveBounds) {
			if (auto* src = sourceGlyph(g.name)) {
				src->width = maxX + rsb;
			}
		}
		ImGui::PopID();
	}

	labelRow("Kern");
	for (int i = 0; i + 1 < n; ++i) {
		ImGui::TableSetColumnIndex(i + 1);
		const auto& a = run.glyphs[static_cast<size_t>(i)];
		const auto& b = run.glyphs[static_cast<size_t>(i + 1)];
		float v = rigkit::font::kernValue(ecs, a.name, b.name);
		ImGui::PushID(1000 + i);
		ImGui::SetNextItemWidth(-1.f);
		if (ImGui::InputFloat("##k", &v, 0.f, 0.f, "%.0f")) {
			rigkit::font::setKernPair(ecs, a.name, b.name, v);
			if (selectLeft) {
				*selectLeft = a.name;
			}
			if (selectRight) {
				*selectRight = b.name;
			}
		}
		ImGui::PopID();
	}

	labelRow("Offset");
	for (int i = 0; i < n; ++i) {
		ImGui::TableSetColumnIndex(i + 1);
		ImGui::Text("%.0f", static_cast<double>(run.glyphs[static_cast<size_t>(i)].x));
	}
	ImGui::EndTable();
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
		if (m_left.empty() && m_right.empty()) {
			for (auto e : ecs->registry().view<rigkit::ecs::CFontKern>()) {
				const auto& k = ecs->registry().get<rigkit::ecs::CFontKern>(e);
				selectPair(*ecs, k.left, k.right);
				break;
			}
		}

		// Bottom region: value row + wrap + 72px strip + 6-row metrics table.
		const float previewH = std::clamp(ImGui::GetContentRegionAvail().y * 0.55f, 260.f, 400.f);
		const float listW = std::max(180.f, ImGui::GetContentRegionAvail().x * 0.34f);

		ImGui::BeginChild("kern_pairs", ImVec2(listW, -previewH),
						  ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
		ImGui::TextUnformatted("Pairs");
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##kfilter", "Search", m_filter, sizeof(m_filter));
		int pairCount = 0;
		for (auto e : ecs->registry().view<rigkit::ecs::CFontKern>()) {
			const auto& k = ecs->registry().get<rigkit::ecs::CFontKern>(e);
			char row[160];
			std::snprintf(row, sizeof(row), "%s  %s  %.0f", k.left.c_str(), k.right.c_str(),
						  static_cast<double>(k.value));
			if (m_filter[0] != '\0' && std::strstr(row, m_filter) == nullptr) {
				continue;
			}
			++pairCount;
			const bool sel = k.left == m_left && k.right == m_right;
			ImGui::PushID(static_cast<int>(entt::to_integral(e)));
			if (ImGui::Selectable(row, sel)) {
				selectPair(*ecs, k.left, k.right);
			}
			ImGui::PopID();
		}
		if (pairCount == 0) {
			ImGui::TextDisabled("No pairs");
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("kern_sides", ImVec2(0.f, -previewH), ImGuiChildFlags_Borders);
		if (ImGui::BeginTabBar("kern_sides_tabs")) {
			if (ImGui::BeginTabItem("Side 1")) {
				drawSide(*ecs, true);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Side 2")) {
				drawSide(*ecs, false);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::EndChild();

		ImGui::BeginChild("kern_edit", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders);
		ImGui::Text("Left  %s", m_left.empty() ? "-" : m_left.c_str());
		ImGui::SameLine(0.f, 24.f);
		ImGui::Text("Right  %s", m_right.empty() ? "-" : m_right.c_str());
		if (!m_left.empty() && !m_right.empty()) {
			const float resolved = rigkit::font::kernValue(*ecs, m_left, m_right);
			const auto exact = rigkit::font::findKernEntity(*ecs, m_left, m_right);
			float v = exact != entt::null ? ecs->getComponent<rigkit::ecs::CFontKern>(exact).value
										  : resolved;
			ImGui::SetNextItemWidth(120.f);
			if (ImGui::InputFloat("Value", &v, 1.f, 10.f, "%.0f")) {
				rigkit::font::setKernPair(*ecs, m_left, m_right, v);
			}
			ImGui::SameLine();
			if (exact == entt::null && std::fabs(resolved) > 0.5f) {
				ImGui::TextDisabled("resolved %.0f", static_cast<double>(resolved));
			}
		}
		ImGui::SetNextItemWidth(48.f);
		if (ImGui::InputText("Wrap", m_wrap, sizeof(m_wrap))) {
			if (!m_left.empty() && !m_right.empty()) {
				selectPair(*ecs, m_left, m_right);
			}
		}
		const std::string preview = m_app->previewText();
		const auto run =
			rigkit::font::shapeString(*ecs, preview, m_app->applyKern(), m_app->location());
		ImGui::Text("Preview  %s   advance %.0f", preview.c_str(), static_cast<double>(run.width));
		drawShapedPreview(*ecs, run, std::min(72.f, m_app->previewSizePx()), 72.f);
		const std::string oldL = m_left;
		const std::string oldR = m_right;
		drawKernMetricsTable(*ecs, run, &m_left, &m_right);
		if (m_left != oldL || m_right != oldR) {
			selectPair(*ecs, m_left, m_right);
		}
		ImGui::EndChild();
	}

  private:
	void selectPair(rigkit::MEcs& ecs, const std::string& left, const std::string& right) {
		m_left = left;
		m_right = right;
		if (!m_app) {
			return;
		}
		const std::string a = glyphPreviewChar(ecs, left);
		const std::string b = glyphPreviewChar(ecs, right);
		const std::string w = m_wrap;
		if (!a.empty() && !b.empty()) {
			const std::string unit = w + a + b;
			m_app->previewText() = unit + unit + unit;
		}
	}

	void drawSide(rigkit::MEcs& ecs, bool leftSide) {
		const int want = leftSide ? rigkit::ecs::CFontGroup::Left : rigkit::ecs::CFontGroup::Right;
		ImGui::TextUnformatted("Groups");
		for (auto e : ecs.registry().view<rigkit::ecs::CFontGroup>()) {
			const auto& g = ecs.registry().get<rigkit::ecs::CFontGroup>(e);
			if (g.side != want && g.side != rigkit::ecs::CFontGroup::Unclassified) {
				continue;
			}
			const std::string n = glyphLabel(ecs, e);
			if (n.empty()) {
				continue;
			}
			char row[160];
			std::snprintf(row, sizeof(row), "%s  (%zu)", shortGroupName(n).c_str(),
						  g.members.size());
			const bool sel = leftSide ? n == m_left : n == m_right;
			ImGui::PushID(static_cast<int>(entt::to_integral(e)));
			if (ImGui::Selectable(row, sel)) {
				if (leftSide) {
					selectPair(ecs, n, m_right);
				} else {
					selectPair(ecs, m_left, n);
				}
			}
			ImGui::PopID();
			if (sel && !g.members.empty()) {
				ImGui::Indent();
				for (const auto& m : g.members) {
					ImGui::TextDisabled("%s", m.c_str());
				}
				ImGui::Unindent();
			}
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Glyphs");
		if (ImGui::BeginTable(leftSide ? "kg1" : "kg2", 6, ImGuiTableFlags_SizingStretchSame)) {
			for (auto e : ecs.registry().view<rigkit::ecs::CFontGlyph>()) {
				const std::string n = glyphLabel(ecs, e);
				if (n.empty()) {
					continue;
				}
				ImGui::TableNextColumn();
				const bool sel = leftSide ? n == m_left : n == m_right;
				ImGui::PushID(static_cast<int>(entt::to_integral(e)));
				if (ImGui::Selectable(n.c_str(), sel)) {
					if (leftSide) {
						selectPair(ecs, n, m_right);
					} else {
						selectPair(ecs, m_left, n);
					}
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}

	UfomatApp* m_app = nullptr;
	std::string m_left;
	std::string m_right;
	char m_filter[64]{};
	char m_wrap[8]{"n"};
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
		drawShapedPreview(*ecs, run, m_app->previewSizePx(),
						  std::max(64.f, ImGui::GetContentRegionAvail().y * 0.45f));
		drawKernMetricsTable(*ecs, run);
	}

  private:
	UfomatApp* m_app = nullptr;
};

} // namespace

UfomatApp::UfomatApp() { settings().appName = "ufomat"; }

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
	if (m_layer == entt::null || !ecs->registry().valid(m_layer) ||
		!ecs->hasComponent<rigkit::ecs::CFontLayer>(m_layer)) {
		m_layer = entt::null;
		for (auto e : ecs->registry().view<rigkit::ecs::CFontLayer>()) {
			m_layer = e;
			if (ecs->registry().get<rigkit::ecs::CFontLayer>(e).role ==
				rigkit::ecs::CFontLayer::Foreground) {
				break;
			}
		}
	}
	for (auto e : ecs->registry().view<rigkit::ecs::CFontGlyph>()) {
		if (m_layer != entt::null &&
			(!ecs->hasComponent<rigkit::ecs::CRelationship>(e) ||
			 ecs->getComponent<rigkit::ecs::CRelationship>(e).parent != m_layer)) {
			continue;
		}
		selectGlyph(e);
		return;
	}
	m_glyph = entt::null;
	m_face = entt::null;
}

void UfomatApp::selectLayer(entt::entity layer) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	m_layer = layer;
	std::string name;
	if (m_glyph != entt::null && ecs->registry().valid(m_glyph)) {
		name = rigkit::font::stableName(*ecs, m_glyph);
	}
	auto next = rigkit::font::findGlyphInLayer(*ecs, layer, name);
	if (next != entt::null) {
		selectGlyph(next);
	} else {
		m_glyph = entt::null;
		selectFirstGlyph();
	}
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
	if (auto* settings = m_engine->getSettingsManager()) {
		settings->registerPreferences("ufomat.surface", "Glyph Surface", &m_surfacePrefs);
	}
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
			wm->createWindow<FontInfoPanel>(this);
			wm->createWindow<LayersPanel>(this);
			wm->showWindow("Glyphs");
			wm->showWindow("Layers");
			wm->showWindow("Glyph");
			wm->showWindow("Properties");
			wm->showWindow("Kern");
			wm->showWindow("Features");
			wm->showWindow("Preview");
			wm->showWindow("Font Info");
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
			ImGuiID leftBottom = 0;
			ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.3f, &leftBottom, &left);
			ImGui::DockBuilderDockWindow("Glyphs", left);
			ImGui::DockBuilderDockWindow("Layers", leftBottom);
			ImGui::DockBuilderDockWindow("Glyph", center);
			ImGui::DockBuilderDockWindow("Properties", right);
			ImGui::DockBuilderDockWindow("Font Info", right);
			ImGui::DockBuilderDockWindow("Kern", bottom);
			ImGui::DockBuilderDockWindow("Features", bottom);
			ImGui::DockBuilderDockWindow("Preview", bottom);
			ImGui::DockBuilderFinish(dockspaceId);
			mui->setDockLayoutBuilder(nullptr);
		});
	}
}
