#pragma once

#include "core/U_core.h"

class FontFaceApp : public rigkit::IApp {
  public:
	FontFaceApp();
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
