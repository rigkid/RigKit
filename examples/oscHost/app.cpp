#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigOsc/src/CNetworkIdentity.h"
#include "packs/rigOsc/src/COscEndpoint.h"
#include "packs/rigOsc/src/rigOsc.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "ToolControlWindow.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <spdlog/spdlog.h>
#include <variant>

namespace {

bool argFloat(const rigkit::osc::OscArg& a, float& out) {
	if (const auto* f = std::get_if<float>(&a)) {
		out = *f;
		return true;
	}
	if (const auto* d = std::get_if<double>(&a)) {
		out = static_cast<float>(*d);
		return true;
	}
	if (const auto* i = std::get_if<int32_t>(&a)) {
		out = static_cast<float>(*i);
		return true;
	}
	if (const auto* h = std::get_if<int64_t>(&a)) {
		out = static_cast<float>(*h);
		return true;
	}
	return false;
}

bool argBool(const rigkit::osc::OscArg& a, bool& out) {
	if (const auto* b = std::get_if<bool>(&a)) {
		out = *b;
		return true;
	}
	if (const auto* i = std::get_if<int32_t>(&a)) {
		out = *i != 0;
		return true;
	}
	if (const auto* f = std::get_if<float>(&a)) {
		out = *f >= 0.5f;
		return true;
	}
	return false;
}

} // namespace

OscHost::OscHost() {
	// Compact default - two instances fit on one desktop.
	window().width = 640;
	window().height = 400;
	window().title = "RigKit - oscHost";
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
		std::cout << "oscHost - RigKit OSC show host (multi-instance via network id)\n\n"
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
		spdlog::warn("Author mode but no IMui - rigImGui failed to attach");
		return;
	}

	auto* mui = dynamic_cast<rigkit::Mui*>(ui);
	if (mui) {
		mui->setDockPassthroughCentral(true);
		mui->addAllHostPanels();
		mui->setWindowVisibility("Debug", true);
		mui->showNotification("Author mode - oscHost");
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

void OscHost::registerShowBus() {
	if (!m_osc) {
		return;
	}
	m_osc->addBusListener("master", [this](const rigkit::osc::OscRx& rx) {
		float v = 0.f;
		if (!rx.msg.args.empty() && argFloat(rx.msg.args[0], v)) {
			m_masterLevel = std::clamp(v, 0.f, 1.f);
		}
	});
	m_osc->addBusListener("blackout", [this](const rigkit::osc::OscRx& rx) {
		bool v = false;
		if (!rx.msg.args.empty() && argBool(rx.msg.args[0], v)) {
			m_blackout = v;
		}
	});
	m_osc->addBusListener("color", [this](const rigkit::osc::OscRx& rx) {
		if (rx.msg.args.size() < 3) {
			return;
		}
		float r = 0.f, g = 0.f, b = 0.f;
		if (argFloat(rx.msg.args[0], r) && argFloat(rx.msg.args[1], g) &&
			argFloat(rx.msg.args[2], b)) {
			m_color[0] = std::clamp(r, 0.f, 1.f);
			m_color[1] = std::clamp(g, 0.f, 1.f);
			m_color[2] = std::clamp(b, 0.f, 1.f);
		}
	});
	m_osc->addBusListener("status", [this](const rigkit::osc::OscRx& rx) {
		if (rx.msg.args.empty()) {
			return;
		}
		if (const auto* s = std::get_if<std::string>(&rx.msg.args[0])) {
			m_showStatus = *s;
		}
	});
	m_osc->addBusListener("heartbeat", [](const rigkit::osc::OscRx&) {
		// Local counter is ours. A peer tick must not replace it.
	});
}

void OscHost::publishShowBus() {
	if (!m_osc || !m_osc->endpoint().sendEnabled) {
		return;
	}
	const std::string& id = m_osc->identity().networkId;
	m_osc->send(m_osc->makeAddress("heartbeat"), {static_cast<int32_t>(m_showHeartbeat), id});
	m_osc->send(m_osc->makeAddress("status"), {m_showStatus, id});
	m_osc->send(m_osc->makeAddress("master"), {m_masterLevel, id});
	m_osc->send(m_osc->makeAddress("blackout"), {m_blackout, id});
	m_osc->send(m_osc->makeAddress("color"), {m_color[0], m_color[1], m_color[2], id});
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
		spdlog::error("oscHost: OSC bind failed - {}", m_osc->lastError());
	}
	if (m_osc) {
		const std::string title =
			std::string("RigKit - oscHost [") + m_osc->identity().networkId + "]";
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

	registerShowBus();

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
		publishShowBus();
	}
}

void OscHost::draw() {
	float level = m_blackout ? 0.f : m_masterLevel;
	getEngine()->setClearColor(m_color[0] * level, m_color[1] * level, m_color[2] * level, 1.f);
}

void OscHost::exit() {
	spdlog::info("oscHost exit");
}
