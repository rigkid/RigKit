#pragma once

#include <memory>
#include <string>

#include "core/U_core.h"

namespace rigkit {
class rigPlotter;
class rigProject;
} // namespace rigkit

class SvgEditorApp : public rigkit::IApp {
  public:
	SvgEditorApp();
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float) override;
	void draw() override {}

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	void seedDemoLayers();
	bool runSmoke();
	std::string documentPathHint() const;

	std::shared_ptr<rigkit::rigPlotter> m_plotter;
	std::shared_ptr<rigkit::rigProject> m_document;
	bool m_smoke = false;
	bool m_smokeOk = false;
};
