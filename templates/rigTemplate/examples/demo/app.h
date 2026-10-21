#pragma once
#include "core/U_core.h"
class DemoApp : public rigkit::IApp {
  public:
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
