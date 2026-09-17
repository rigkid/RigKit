#pragma once

#include <entt/entt.hpp>
#include <string>
#include <unordered_map>

#include "core/U_core.h"
#include "core/util/UndoStack.h"
#include "packs/rigPlotComponent/src/CArtboard.h"
#include "packs/rigPlotComponent/src/CBedView.h"

class UfomatApp : public rigkit::IApp {
  public:
	UfomatApp();
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float) override {}
	void draw() override {}

	void selectGlyph(entt::entity glyph);
	void selectFace(entt::entity face);
	void promptOpen();
	void promptSave();
	void promptExportTtf();
	void openUfo(const std::string& path);
	void saveUfo(const std::string& path);
	bool runSmoke();

	entt::entity selectedGlyph() const { return m_glyph; }
	entt::entity selectedFace() const { return m_face; }
	entt::entity ensureFeatures();
	rigkit::ecs::CBedView& bedView() { return m_view; }
	rigkit::ecs::CArtboard& board() { return m_board; }
	void syncBoardFromFace();

	std::string& previewText() { return m_previewText; }
	float& previewSizePx() { return m_previewSizePx; }
	bool& applyKern() { return m_applyKern; }
	std::unordered_map<std::string, float>& location() { return m_location; }

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	void registerPacks();
	void loadContractDemo();
	void selectFirstGlyph();
	bool runVarSmoke();
	bool runTtfSmoke();

	rigkit::ecs::CBedView m_view;
	rigkit::ecs::CArtboard m_board;
	rigkit::UndoStack m_undo;
	entt::entity m_glyph{entt::null};
	entt::entity m_face{entt::null};
	std::string m_previewText = "AV";
	float m_previewSizePx = 72.f;
	bool m_applyKern = true;
	std::unordered_map<std::string, float> m_location;
	bool m_smoke = false;
	bool m_smokeTtf = false;
	bool m_smokeOk = false;
};
