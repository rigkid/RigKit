#pragma once

#include "core/IApp.h"

namespace rigkit {

/** Minimal IApp for headless contract_smoke — no window, no UI. */
class SmokeApp : public IApp {
  public:
	void setup() override {}
	void update(float) override {}
	void draw() override {}
	void exit() override {}
};

} // namespace rigkit
