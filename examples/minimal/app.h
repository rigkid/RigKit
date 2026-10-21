#pragma once
#include "core/U_core.h"
class MinimalApp : public rigkit::IApp {
  public:
	void setup() override;
	void update(float) override {}
	void draw() override {
		// Host Draw presents ECS shapes/meshes created in setup().
	}
};
