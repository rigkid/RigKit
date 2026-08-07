#pragma once
#include "core/U_core.h"

class MyApp : public rigkit::IApp {
  public:
	MyApp() {
		window().width = 800;
		window().height = 600;
		window().title = "MyApp";
	}
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
