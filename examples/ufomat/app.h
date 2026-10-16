#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/U_core.h"
#include "core/util/UndoStack.h"
#include "ecs/PropertyReflection.h"
#include "packs/rigPlotComponent/src/CArtboard.h"
#include "packs/rigPlotComponent/src/CBedView.h"

/**
 * @brief UFOMAT glyph-surface chrome prefs (not `rig.font.*`, not CAD gizmos).
 * @details Persisted as `ufomat.surface`. Path node type is inferred from handles.
 */
struct UfomatSurfacePrefs {
	bool showHandles = true;
	bool handlesOnSelectedOnly = false;
	float handlePx = 3.5f;
	float nodePx = 4.f;
	/// 0 = Circle, 1 = Square
	int handleShape = 0;
	/// 0 = Auto, 1 = Circles, 2 = Squares
	int nodeTypeDisplay = 0;
	/// 0 = Smooth, 1 = Corner (new points; double-click still toggles)
	int defaultNodeType = 0;
	bool showMetricLabels = true;
	bool showMetricValues = true;
	bool showUfoAnchors = true;
	/// Ghost the same glyph from the other layers (masters, background).
	bool showOtherLayers = true;
	glm::vec4 path{0.15f, 1.f, 0.28f, 1.f};
	glm::vec4 metrics{0.15f, 0.85f, 0.28f, 0.55f};
	glm::vec4 labels{0.2f, 1.f, 0.35f, 0.9f};
	glm::vec4 handle{0.2f, 1.f, 0.4f, 0.85f};
	glm::vec4 node{0.2f, 1.f, 0.35f, 1.f};
	glm::vec4 nodeSelected{0.45f, 1.f, 0.55f, 1.f};
	glm::vec4 background{0.f, 0.04f, 0.f, 1.f};
	glm::vec4 ghost{0.5f, 0.75f, 0.6f, 0.35f};

	std::vector<sProp> GetProperties() {
		static constexpr const char* kHandleShape[] = {"Circle", "Square"};
		static constexpr const char* kNodeDisplay[] = {"Auto", "Circles", "Squares"};
		static constexpr const char* kNodeType[] = {"Smooth", "Corner"};
		return {
			{0, "Show Handles", EPT_BOOL, &showHandles},
			{1, "Handles On Selected Only", EPT_BOOL, &handlesOnSelectedOnly},
			{2, "Handle Size", EPT_FLOAT, &handlePx},
			{3, "Node Size", EPT_FLOAT, &nodePx},
			{4, "Handle Shape", EPT_ENUM, &handleShape, kHandleShape, 2},
			{5, "Node Type Display", EPT_ENUM, &nodeTypeDisplay, kNodeDisplay, 3},
			{6, "Default Node Type", EPT_ENUM, &defaultNodeType, kNodeType, 2},
			{7, "Show Metric Labels", EPT_BOOL, &showMetricLabels},
			{8, "Show Metric Values", EPT_BOOL, &showMetricValues},
			{9, "Show UFO Anchors", EPT_BOOL, &showUfoAnchors},
			{10, "Show Other Layers", EPT_BOOL, &showOtherLayers},
			{11, "Path", EPT_COLOR, &path},
			{12, "Metrics", EPT_COLOR, &metrics},
			{13, "Labels", EPT_COLOR, &labels},
			{14, "Handle", EPT_COLOR, &handle},
			{15, "Node", EPT_COLOR, &node},
			{16, "Node Selected", EPT_COLOR, &nodeSelected},
			{17, "Background", EPT_COLOR, &background},
			{18, "Ghost", EPT_COLOR, &ghost},
		};
	}
};

class UfomatApp : public rigkit::IApp {
  public:
	UfomatApp();
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float) override {}
	void draw() override {}

	void selectGlyph(entt::entity glyph);
	void selectFace(entt::entity face);
	/// Make @p layer current; reselects the same-named glyph on that layer.
	void selectLayer(entt::entity layer);
	void promptOpen();
	void promptSave();
	void promptExportTtf();
	void openUfo(const std::string& path);
	void saveUfo(const std::string& path);
	bool runSmoke();

	entt::entity selectedGlyph() const { return m_glyph; }
	entt::entity selectedFace() const { return m_face; }
	entt::entity activeLayer() const { return m_layer; }
	entt::entity ensureFeatures();
	rigkit::ecs::CBedView& bedView() { return m_view; }
	rigkit::ecs::CArtboard& board() { return m_board; }
	void syncBoardFromFace();

	std::string& previewText() { return m_previewText; }
	float& previewSizePx() { return m_previewSizePx; }
	bool& applyKern() { return m_applyKern; }
	std::unordered_map<std::string, float>& location() { return m_location; }
	UfomatSurfacePrefs& surfacePrefs() { return m_surfacePrefs; }
	const UfomatSurfacePrefs& surfacePrefs() const { return m_surfacePrefs; }

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	void registerPacks();
	void loadContractDemo();
	void selectFirstGlyph();
	bool runVarSmoke();
	bool runTtfSmoke();

	rigkit::ecs::CBedView m_view;
	rigkit::ecs::CArtboard m_board;
	UfomatSurfacePrefs m_surfacePrefs;
	rigkit::UndoStack m_undo;
	entt::entity m_glyph{entt::null};
	entt::entity m_face{entt::null};
	entt::entity m_layer{entt::null};
	std::string m_previewText = "AV";
	float m_previewSizePx = 72.f;
	bool m_applyKern = true;
	std::unordered_map<std::string, float> m_location;
	bool m_smoke = false;
	bool m_smokeTtf = false;
	bool m_smokeOk = false;
};
