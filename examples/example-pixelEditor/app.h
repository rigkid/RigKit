#pragma once

#include "core/U_core.h"

#include <memory>
#include <string>

namespace rigkit {
class Mui;
class rigPixelPlotter;
class rigPixelPlotterUi;
class rigProject;
} // namespace rigkit

class PixelEditorApp : public rigkit::IApp {
  public:
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float) override {}
	void onUiAttached() override;

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }
	bool batchFailed() const { return m_batch && !m_batchOk; }

  private:
	/** @brief Demo document for --smoke only; interactive start stays blank. */
	void seedSmokeDocument();
	bool runSmoke();
	/**
	 * @brief Apply a saved `.rig` recipe to every still in `--in`, write one
	 * `.rigz` smart object per file into `--out`.
	 * @details Each package embeds the original scan as a `data/links/` sidecar
	 * (source files on disk are never overwritten) plus the recipe
	 * Curves/Levels stack. Place those `.rigz`s in Relay; export PNG/PDF from
	 * the picture-book document after type — not from this batch.
	 */
	bool runBatch();

	rigkit::Mui* m_mui = nullptr;
	std::shared_ptr<rigkit::rigPixelPlotter> m_plotter;
	std::shared_ptr<rigkit::rigPixelPlotterUi> m_uiPack;
	std::shared_ptr<rigkit::rigProject> m_document;
	bool m_smoke = false;
	bool m_smokeOk = false;
	bool m_batch = false;
	bool m_batchOk = false;
	std::string m_recipePath;
	std::string m_batchIn;
	std::string m_batchOut;
};
