#pragma once

#include "core/U_core.h"

class SvgAnimatorApp : public rigkit::IApp {
  public:
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float) override {}
	void draw() override {}

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	bool loadDemo();
	bool runSmoke();

	bool m_smoke = false;
	bool m_smokeOk = false;
};
