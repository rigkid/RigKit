#pragma once
#include "core/U_core.h"
class ClockApp : public rigkit::IApp {
  public:
	ClockApp() {
		window().width = 800;
		window().height = 600;
		window().title = "rigMusicComponent - clock";
	}
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
