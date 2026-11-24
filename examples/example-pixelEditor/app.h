#pragma once

#include "core/U_core.h"

#include <memory>

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

  private:
	/** @brief Demo document for --smoke only; interactive start stays blank. */
	void seedSmokeDocument();
	bool runSmoke();

	rigkit::Mui* m_mui = nullptr;
	std::shared_ptr<rigkit::rigPixelPlotter> m_plotter;
	std::shared_ptr<rigkit::rigPixelPlotterUi> m_uiPack;
	std::shared_ptr<rigkit::rigProject> m_document;
	bool m_smoke = false;
	bool m_smokeOk = false;
};
