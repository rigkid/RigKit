#pragma once

#include "core/U_core.h"

#include <memory>

namespace rigkit {
class rigPixelPlotter;
class rigProject;
} // namespace rigkit

class PixelEditorApp : public rigkit::IApp {
  public:
	PixelEditorApp();
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float) override {}
	void draw() override {}

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	void seedDocument();
	bool runSmoke();

	std::shared_ptr<rigkit::rigPixelPlotter> m_plotter;
	std::shared_ptr<rigkit::rigProject> m_document;
	bool m_smoke = false;
	bool m_smokeOk = false;
};
