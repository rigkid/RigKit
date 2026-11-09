#include <Arduino.h>

#include "App.h"

static rigkit::espapp::App gApp;

void setup() {
	Serial.begin(115200);
	delay(200);
	rigkit::espapp::Setup(gApp);
}

void loop() {
	static uint32_t lastMs = millis();
	const uint32_t nowMs = millis();
	const float dt = static_cast<float>(nowMs - lastMs) * 0.001f;
	lastMs = nowMs;
	rigkit::espapp::Update(gApp, dt);
	rigkit::espapp::Draw(gApp);
	delay(16);
}
