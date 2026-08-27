#pragma once
#include "core/U_core.h"
class DemoApp : public rigkit::IApp {
  public:
	DemoApp() {
		window().width = 800;
		window().height = 600;
		window().title = "rigTemplate - demo";
	}
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
