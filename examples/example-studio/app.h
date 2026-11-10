#pragma once

#include "core/U_core.h"

#include <memory>

namespace rigkit {
class Mui;
class rigPixelPlotter;
class rigPlotter;
class rigProject;
} // namespace rigkit

class StudioApp : public rigkit::IApp {
  public:
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float) override {}
	void draw() override {}
	void onUiAttached() override;

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	void seedPixel();
	void seedVector();
	bool runSmoke();

	rigkit::Mui* m_mui = nullptr;
	std::shared_ptr<rigkit::rigPlotter> m_plotter;
	std::shared_ptr<rigkit::rigPixelPlotter> m_pixel;
	std::shared_ptr<rigkit::rigProject> m_document;
	bool m_smoke = false;
	bool m_smokeOk = false;
};
