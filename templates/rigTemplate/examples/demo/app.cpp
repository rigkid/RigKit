#include "app.h"
#include "core/RigKitEngine.h"

void DemoApp::setup() {
	spdlog::info("demo - rename this example with your pack; register the pack in setup()");
	m_engine->setClearColor(0.12f, 0.14f, 0.18f, 1.0f);
}
