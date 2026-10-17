#pragma once
#include "core/U_core.h"
class CadBoxApp : public rigkit::IApp {
  public:
	CadBoxApp() {
		window().width = 800;
		window().height = 600;
		window().title = "rigCadComponent - cadBox";
	}
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
