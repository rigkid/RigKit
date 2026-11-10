#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "ecs/MEcs.h"
#include "ecs/systems/SEvent.h"
#include "packs/rigComponent/src/CDrawStyle.h"
#include "packs/rigComponent/src/CRenderBlend.h"
#include "packs/rigComponent/src/CUiPanel.h"
#include "packs/rigComponent/src/rig/create.h"
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
	settings().appName = "oscHost";
}

rigkit::ecs::CDrawStyle& OscHost::lookStyle() const {
	return getEngine()->getECSManager()->getComponent<rigkit::ecs::CDrawStyle>(m_look);
}

rigkit::ecs::CRenderBlend& OscHost::lookBlend() const {
	return getEngine()->getECSManager()->getComponent<rigkit::ecs::CRenderBlend>(m_look);
}

float& OscHost::masterLevel() {
	return lookBlend().opacity;
}

float OscHost::masterLevel() const {
	return lookBlend().opacity;
}

float* OscHost::color() {
	return &lookStyle().fillR;
}

void OscHost::registerUIActions(rigkit::ecs::SEvent& events) {
	events.registerAction("look.reset", [this]() {
		if (m_look == entt::null || !getEngine() || !getEngine()->getECSManager()) {
			return;
		}
		lookStyle().setFillColor(0.35f, 0.55f, 0.95f);
		lookBlend().opacity = 1.f;
		m_blackout = false;
	});
}

void OscHost::setupLook() {
	auto* ecs = getEngine()->getECSManager();
	m_look = ecs->createEntity("look");
	auto& style = ecs->addComponent<rigkit::ecs::CDrawStyle>(m_look);
	style.setFillColor(0.35f, 0.55f, 0.95f);
	style.hasStroke = false;
	auto& blend = ecs->addComponent<rigkit::ecs::CRenderBlend>(m_look);
	blend.opacity = 1.f;
}

void OscHost::setupLookPanel() {
	auto* ecs = getEngine()->getECSManager();
	using Surface = rigkit::ecs::CUiPanel::Surface;
	using Widget = rigkit::ecs::CUiControl::Widget;
	const auto panel = rig::makeUiPanel(*ecs, "show.look", Surface::Tool, "Look");
	auto bindSlider = [&](const char* key, int order, const char* name) {
		const auto e = rig::makeUiControl(*ecs, panel, m_look, key, order, Widget::Slider, name);
		auto& c = ecs->getComponent<rigkit::ecs::CUiControl>(e);
		c.min = 0.f;
		c.max = 1.f;
		c.step = 0.01f;
		c.type = "float";
	};
	bindSlider("Opacity", 0, "Master");
	bindSlider("Fill R", 1, "Red");
	bindSlider("Fill G", 2, "Green");
	bindSlider("Fill B", 3, "Blue");
	rig::makeUiAction(*ecs, panel, "look.reset", 4, "Reset look");
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
	setupLook();
}

void OscHost::onUiAttached() {
	m_mui = getEngine() ? rigkit::authorMui(*getEngine()) : nullptr;
}

void OscHost::setupAuthorUi() {
	m_mui = getEngine() ? rigkit::authorMui(*getEngine()) : nullptr;
	if (!m_mui) {
		spdlog::warn("Author mode but no Mui - rigImGui failed to attach");
		return;
	}
	m_mui->setDockPassthroughCentral(true);
	m_mui->addAllHostPanels();
	m_mui->setWindowVisibility("Debug", true);
	m_mui->showNotification("Author mode - oscHost");

	auto* wm = m_mui->getWindowManager();
	if (!wm) {
		return;
	}

	wm->createWindow<ToolControlWindow>(this);
	wm->showWindow("Show Control");
	setupLookPanel();
	m_mui->setFirstRunHostDockLayout({"Show Control"});
	spdlog::info("Author UI: rigImGui host shell + Show Control + Look panel");
}

void OscHost::registerShowBus() {
	if (!m_osc) {
		return;
	}
	m_osc->addBusListener("master", [this](const rigkit::osc::OscRx& rx) {
		float v = 0.f;
		if (!rx.msg.args.empty() && argFloat(rx.msg.args[0], v)) {
			masterLevel() = std::clamp(v, 0.f, 1.f);
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
			float* c = color();
			c[0] = std::clamp(r, 0.f, 1.f);
			c[1] = std::clamp(g, 0.f, 1.f);
			c[2] = std::clamp(b, 0.f, 1.f);
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
	const float* c = color();
	m_osc->send(m_osc->makeAddress("master"), {masterLevel(), id});
	m_osc->send(m_osc->makeAddress("blackout"), {m_blackout, id});
	m_osc->send(m_osc->makeAddress("color"), {c[0], c[1], c[2], id});
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
	if (m_look == entt::null) {
		return;
	}
	const float level = m_blackout ? 0.f : masterLevel();
	const float* c = color();
	getEngine()->setClearColor(c[0] * level, c[1] * level, c[2] * level, 1.f);
}

void OscHost::exit() {
	spdlog::info("oscHost exit");
}

