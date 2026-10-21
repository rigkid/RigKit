#include "rendering/U_gladGlfw.h"

#include "app.h"

#include "core/RigKitEngine.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "ecs/MEcs.h"
#include "ecs/SystemRegistry.h"
#include "rendering/IRenderer.h"
#include "SchDoc.h"

#include <spdlog/spdlog.h>

namespace {
const float kGrids[] = {0.254f, 1.27f, 2.54f};
}

KicadSchApp::KicadSchApp() : m_doc(new SchDoc) {}

KicadSchApp::~KicadSchApp() {
	delete m_doc;
}

void KicadSchApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	m_smoke = args.hasFlag("smoke");
	if (auto smoked = args.getValue("smoke")) {
		m_smoke = true;
		if (!smoked->empty()) {
			m_path = *smoked;
		}
	}
	const auto& pos = args.getPositionalArgs();
	if (!pos.empty()) {
		m_path = pos.front();
	}
}

void KicadSchApp::setup() {
	m_engine->setClearColor(0.75f, 0.75f, 0.78f, 1.f);
	if (m_path.empty()) {
		m_path = AppPaths::joinPath(AppPaths::getDataDir(), "led.kicad_sch");
	}
	const bool loaded = m_doc->load(m_path);
	if (!loaded) {
		spdlog::error("{}", m_doc->error());
	}
	m_doc->setGrid(kGrids[m_gridIndex]);

	if (m_smoke) {
		m_smokeOk = loaded && m_doc->connectionsSurviveGridChange() &&
					m_doc->probeInstalledLibraries() && m_doc->placeSmoke() &&
					m_doc->hierarchySmoke();
		if (!m_smokeOk) {
			spdlog::error("example-kicadSch --smoke failed: {}", m_doc->error());
		} else {
			spdlog::info("example-kicadSch --smoke ok");
		}
		if (auto* win = m_engine->getWindow()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
		return;
	}

	if (auto* ecs = m_engine->getECSManager()) {
		ecs->registerSystem("SchPresent", rigkit::SystemPhase::Draw, [this] { present(); });
	}
	if (GLFWwindow* win = m_engine->getWindow()) {
		glfwSetWindowTitle(win, m_doc->path().c_str());
		glfwSetWindowUserPointer(win, this);
		glfwSetScrollCallback(win, [](GLFWwindow* w, double, double yoff) {
			if (auto* app = static_cast<KicadSchApp*>(glfwGetWindowUserPointer(w))) {
				app->addWheel(static_cast<float>(yoff));
			}
		});
		glfwSetCharCallback(win, [](GLFWwindow* w, unsigned int cp) {
			if (auto* app = static_cast<KicadSchApp*>(glfwGetWindowUserPointer(w))) {
				if (cp >= 32 && cp < 127) {
					app->addChar(static_cast<char>(cp));
				}
			}
		});
	}
	spdlog::info("Click a sheet box to open it, H lists the project sheets, Backspace goes up. "
				 "A places a symbol. Drag keeps wires on pins. [ ] grid  wheel zoom  middle pan  "
				 "F fit  S save");
}

bool KicadSchApp::pointer(float& sx, float& sy, float& kx, float& ky) {
	GLFWwindow* win = m_engine ? m_engine->getWindow() : nullptr;
	if (!win) {
		return false;
	}
	double mx = 0.0;
	double my = 0.0;
	glfwGetCursorPos(win, &mx, &my);
	int designW = 0;
	int designH = 0;
	int fbW = 0;
	int fbH = 0;
	m_engine->getPresentSize(designW, designH, fbW, fbH);
	const int winW = m_engine->getWindowWidth();
	const int winH = m_engine->getWindowHeight();
	sx = static_cast<float>(mx);
	sy = static_cast<float>(my);
	if (winW > 0 && designW > 0) {
		sx *= static_cast<float>(designW) / static_cast<float>(winW);
	}
	if (winH > 0 && designH > 0) {
		sy *= static_cast<float>(designH) / static_cast<float>(winH);
	}
	m_view.canvasW = static_cast<float>(designW > 0 ? designW : 1);
	m_view.canvasH = static_cast<float>(designH > 0 ? designH : 1);
	m_view.contentSize = {m_doc->pageW(), m_doc->pageH()};
	m_view.updateDerived();
	const glm::vec2 c = m_view.toContent(sx, sy);
	kx = c.x;
	ky = m_doc->pageH() - c.y;
	return true;
}

void KicadSchApp::onKey(int key) {
	if (key == GLFW_KEY_LEFT_BRACKET || key == GLFW_KEY_RIGHT_BRACKET) {
		if (key == GLFW_KEY_LEFT_BRACKET) {
			m_gridIndex = (m_gridIndex + 2) % 3;
		} else {
			m_gridIndex = (m_gridIndex + 1) % 3;
		}
		m_doc->setGrid(kGrids[m_gridIndex]);
		spdlog::info("grid {} mm - existing wires stay put", m_doc->grid());
		return;
	}
	if (key == GLFW_KEY_F) {
		m_view.fitToCanvas();
		return;
	}
	if (key == GLFW_KEY_S) {
		if (!m_doc->save(m_doc->path())) {
			spdlog::error("{}", m_doc->error());
		}
	}
}

void KicadSchApp::update(float) {
	if (m_smoke || !m_engine || !m_engine->getWindow()) {
		return;
	}
	if (m_doc->consumeRefit()) {
		m_fitted = false;
		if (GLFWwindow* win = m_engine->getWindow()) {
			glfwSetWindowTitle(win, m_doc->path().c_str());
		}
	}
	float sx = 0.f;
	float sy = 0.f;
	float kx = 0.f;
	float ky = 0.f;
	if (!pointer(sx, sy, kx, ky)) {
		return;
	}
	if (!m_fitted && m_view.canvasW > 1.f) {
		m_view.fitToCanvas();
		m_fitted = true;
	}
	if (m_wheel != 0.f) {
		m_view.applyScrollZoom(m_wheel, sx, sy);
		m_wheel = 0.f;
	}

	GLFWwindow* win = m_engine->getWindow();
	const bool mid = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
	const bool right = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
	const bool left = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	if ((mid || right) && m_panning) {
		m_view.applyPanDelta(sx - m_lastSx, sy - m_lastSy);
	}
	m_panning = mid || right;

	m_doc->aim(kx, ky);

	auto edge = [&](int key, bool& was) {
		const bool down = glfwGetKey(win, key) == GLFW_PRESS;
		const bool fire = down && !was;
		was = down;
		return fire;
	};
	if (edge(GLFW_KEY_A, m_keyA) && !m_doc->choosing()) {
		m_doc->openChooser();
		m_ignoreA = true;
	}
	if (!m_doc->choosing() && edge(GLFW_KEY_H, m_keyH)) {
		m_doc->toggleSheets();
	}
	if (m_ignoreA && glfwGetKey(win, GLFW_KEY_A) == GLFW_RELEASE) {
		m_ignoreA = false;
	}
	if (m_doc->choosing()) {
		for (char c : m_typed) {
			if (m_ignoreA && (c == 'a' || c == 'A')) {
				continue;
			}
			m_doc->typeChar(c);
		}
		if (edge(GLFW_KEY_ESCAPE, m_keyEsc)) {
			m_doc->closeChooser();
		}
		const bool enterDown = glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS ||
							   glfwGetKey(win, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
		if (enterDown && !m_keyEnter) {
			m_doc->confirmChoice();
		}
		m_keyEnter = enterDown;
		if (edge(GLFW_KEY_BACKSPACE, m_keyBack)) {
			m_doc->backspace();
		}
		if (edge(GLFW_KEY_UP, m_keyUp)) {
			m_doc->moveChoice(-1);
		}
		if (edge(GLFW_KEY_DOWN, m_keyDown)) {
			m_doc->moveChoice(1);
		}
	} else if (m_doc->sheetsOpen()) {
		if (edge(GLFW_KEY_ESCAPE, m_keyEsc)) {
			m_doc->toggleSheets();
		}
		const bool enterDown = glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS ||
							   glfwGetKey(win, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
		if (enterDown && !m_keyEnter) {
			m_doc->confirmSheet();
		}
		m_keyEnter = enterDown;
		if (edge(GLFW_KEY_BACKSPACE, m_keyBack)) {
			m_doc->goBack();
		}
		if (edge(GLFW_KEY_UP, m_keyUp)) {
			m_doc->moveSheet(-1);
		}
		if (edge(GLFW_KEY_DOWN, m_keyDown)) {
			m_doc->moveSheet(1);
		}
	} else {
		if (edge(GLFW_KEY_ESCAPE, m_keyEsc)) {
			m_doc->cancelPlace();
		}
		if (edge(GLFW_KEY_LEFT_BRACKET, m_bracketLeft)) {
			onKey(GLFW_KEY_LEFT_BRACKET);
		}
		if (edge(GLFW_KEY_RIGHT_BRACKET, m_bracketRight)) {
			onKey(GLFW_KEY_RIGHT_BRACKET);
		}
		if (edge(GLFW_KEY_F, m_keyF)) {
			onKey(GLFW_KEY_F);
		}
		if (edge(GLFW_KEY_S, m_keyS)) {
			onKey(GLFW_KEY_S);
		}
		if (edge(GLFW_KEY_BACKSPACE, m_keyBack)) {
			m_doc->goBack();
		}
		m_keyEnter = glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS ||
					 glfwGetKey(win, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
	}
	m_typed.clear();

	if (left && !m_leftWas) {
		if (m_doc->pressSheets(sx, sy)) {
		} else if (!m_doc->pressChooser(sx, sy)) {
			if (m_doc->placingSymbol()) {
				if (!m_doc->placeAtCursor()) {
					spdlog::error("{}", m_doc->error());
				}
			} else {
				m_doc->press(kx, ky);
			}
		}
	}
	if (left && m_doc->dragging()) {
		m_doc->dragTo(kx, ky);
	}
	if (!left && m_leftWas) {
		m_doc->release();
	}
	m_leftWas = left;
	m_lastSx = sx;
	m_lastSy = sy;
}

void KicadSchApp::present() {
	if (!m_engine || !m_engine->getECSManager()) {
		return;
	}
	IRenderer* renderer = m_engine->getECSManager()->getPresentRenderer();
	if (!renderer) {
		return;
	}
	m_doc->draw(*renderer, m_view);
}
