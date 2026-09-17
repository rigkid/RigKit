#pragma once

#include "core/U_core.h"

class ShapeApp : public rigkit::IApp {
  public:
	ShapeApp();
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
