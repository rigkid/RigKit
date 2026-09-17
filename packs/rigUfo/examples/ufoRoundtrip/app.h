#pragma once

#include "core/U_core.h"

class UfoRoundtripApp : public rigkit::IApp {
  public:
	UfoRoundtripApp();
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
