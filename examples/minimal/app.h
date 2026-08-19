#pragma once
#include "core/U_core.h"
class MinimalApp : public rigkit::IApp {
  public:
	MinimalApp() {
		window().width = 800;
		window().height = 600;
		window().title = "RigKit Minimal";
	}
	void setup() override;
	void update(float) override {}
	void draw() override {
		// Host Draw presents ECS shapes/meshes created in setup().
		// Immediate helpers (optional): #include "author/rigDraw.h"
	}
};
