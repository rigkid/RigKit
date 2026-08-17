#include "app.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigOsc/src/CNetworkIdentity.h"
#include "packs/rigOsc/src/COscShowBus.h"
#include "packs/rigOsc/src/rigOsc.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "ToolControlWindow.h"

OscHost::OscHost() {
	// Compact default — two instances fit on one desktop.
	window().width = 640;
	window().height = 400;
	window().title = "RigKit — oscHost";
	settings().appName = "oscHost";
}

void OscHost::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	rigkit::IApp::parseCommandLineArgs(args);
	m_cliArgs = args;

	if (args.hasFlag("show") || args.hasFlag("install")) {
		m_showMode = true;
	}
	if (args.hasFlag("author") || args.hasFlag("edit")) {
		m_showMode = false;
	}
	if (args.hasFlag("smoke-osc")) {
		m_smokeOsc = true;
		m_showMode = true;
	}

	if (args.hasFlag("help") || args.hasFlag("h")) {
		std::cout << "oscHost — RigKit OSC show host (multi-instance via network id)\n\n"
				  << "  --author / --edit     Author mode (rigImGui) [default]\n"
				  << "  --show / --install    Show mode (UI-light)\n"
				  << rigkit::rigOsc::commandLineHelp() << "\n"
				  << "  --smoke-osc           Bind UDP, loopback master, exit\n"
				  << "  --help\n";
		std::exit(0);
	}

	spdlog::info("Layout mode: {}", m_showMode ? "show" : "author");
}

void OscHost::bootstrapPacks() {
	auto* packs = getEngine()->getPackManager();
	if (!packs) {
		return;
	}

	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->registerPack<rigkit::rigOsc>();
	if (!m_showMode) {
		packs->registerPack<rigkit::rigImGui>();
	}

	packs->initAll();
	packs->setupAll();

	m_osc = packs->getPack<rigkit::rigOsc>();
}

void OscHost::setupAuthorUi() {
	auto* ui = getEngine()->getUiManager();
	if (!ui) {
		spdlog::warn("Author mode but no IMui — rigImGui failed to attach");
		return;
	}

	auto* mui = dynamic_cast<rigkit::Mui*>(ui);
	if (mui) {
		mui->setDockPassthroughCentral(true);
		mui->addAllHostPanels();
		mui->setWindowVisibility("Debug", true);
		mui->showNotification("Author mode — oscHost");
	}

	auto* wm = ui->getWindowManager();
	if (!wm) {
		return;
	}

	wm->createWindow<ToolControlWindow>(this);
	wm->showWindow("Show Control");
	if (mui) {
		mui->setFirstRunHostDockLayout({"Show Control"});
	}
	spdlog::info("Author UI: rigImGui host shell + Show Control");
}

void OscHost::syncOscBus() {
	if (!m_osc) {
		return;
	}
	auto& bus = m_osc->showBus();
	if (bus.masterFromNet) {
		m_masterLevel = bus.master;
		bus.masterFromNet = false;
	} else {
		bus.master = m_masterLevel;
	}
	if (bus.blackoutFromNet) {
		m_blackout = bus.blackout;
		bus.blackoutFromNet = false;
	} else {
		bus.blackout = m_blackout;
	}
	if (bus.colorFromNet) {
		m_color[0] = bus.colorR;
		m_color[1] = bus.colorG;
		m_color[2] = bus.colorB;
		bus.colorFromNet = false;
	} else {
		bus.colorR = m_color[0];
		bus.colorG = m_color[1];
		bus.colorB = m_color[2];
	}
	if (bus.statusFromNet) {
		m_showStatus = bus.status;
		bus.statusFromNet = false;
	} else {
		bus.status = m_showStatus;
	}
	bus.heartbeat = m_showHeartbeat;
}

bool OscHost::runOscSmoke() {
	if (!m_osc) {
		spdlog::error("smoke-osc: rigOsc not registered");
		return false;
	}
	return m_osc->smokeLoopback();
}

void OscHost::setup() {
	spdlog::info("oscHost setup");
	getEngine()->init(window().title);

	bootstrapPacks();
	if (m_osc && !m_osc->applyCommandLine(m_cliArgs)) {
		spdlog::error("oscHost: OSC bind failed — {}", m_osc->lastError());
	}
	if (m_osc) {
		const std::string title =
			std::string("RigKit — oscHost [") + m_osc->identity().networkId + "]";
		window().title = title;
		if (auto* win = getEngine()->getWindow()) {
			glfwSetWindowTitle(win, title.c_str());
		}
	}

	if (m_showMode) {
		getEngine()->detachUiManager();
		m_showStatus = "show";
		spdlog::info("Show mode: rigImGui detached");
	} else {
		setupAuthorUi();
		m_showStatus = "authoring";
	}

	syncOscBus();

	if (m_smokeOsc) {
		const bool ok = runOscSmoke();
		if (!ok) {
			spdlog::error("oscHost --smoke-osc failed: {}", m_osc ? m_osc->lastError() : "no pack");
		} else {
			spdlog::info("oscHost --smoke-osc OK");
		}
		if (auto* win = getEngine()->getWindow()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
		if (!ok) {
			std::exit(1);
		}
	}
}

void OscHost::update(float dt) {
	m_heartbeatAccum += dt;
	if (m_heartbeatAccum >= 1.f) {
		m_heartbeatAccum -= 1.f;
		++m_showHeartbeat;
	}

	syncOscBus();
}

void OscHost::draw() {
	float level = m_blackout ? 0.f : m_masterLevel;
	getEngine()->setClearColor(m_color[0] * level, m_color[1] * level, m_color[2] * level, 1.f);
}

void OscHost::exit() {
	spdlog::info("oscHost exit");
}
