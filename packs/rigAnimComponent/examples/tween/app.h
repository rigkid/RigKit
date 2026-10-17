#pragma once
#include "core/U_core.h"
class TweenApp : public rigkit::IApp {
  public:
	TweenApp() {
		window().width = 800;
		window().height = 600;
		window().title = "rigAnimComponent - tween";
	}
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
