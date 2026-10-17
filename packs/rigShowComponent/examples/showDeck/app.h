#pragma once
#include "core/U_core.h"
class ShowDeckApp : public rigkit::IApp {
  public:
	ShowDeckApp() {
		window().width = 800;
		window().height = 600;
		window().title = "rigShowComponent - showDeck";
	}
	void setup() override;
	void update(float) override {}
	void draw() override {}
};
