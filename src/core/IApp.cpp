#include "core/IApp.h"

#include "core/pack/MPack.h"
#include "core/RigKitEngine.h"
#include "core/util/AppPaths.h"
#include "ecs/MEcs.h"
#include "ecs/systems/SEvent.h"
#include "rendering/FpsOverlay.h"
#include "rendering/IRenderer.h"
#include "rendering/MRendering.h"

namespace rigkit {

// -----------------------------------------------------------------------------
// Template-method public wrappers
// -----------------------------------------------------------------------------

void IApp::rigSetup() {
	// Engine was wired by RigKitEngine before this call (setEngine).
	if (m_engine && m_engine->getECSManager()) {
		registerUIActions(m_engine->getECSManager()->getEventSystem());
	}

	setup();
}

void IApp::rigUpdate(float deltaTime) {
	// User-level update first (app logic sets up what it wants)
	update(deltaTime);

	// Framework-level update (systems process user intent)
	// 1) Update ECS-related systems (canvas, scripts, generic systems)
	if (auto ecs = m_engine->getECSManager()) {
		ecs->updateSystems(deltaTime);
	}

	// 2) Packs (rigImGui, show/tool packs, ...)
	if (auto packs = m_engine->getPackManager()) {
		packs->updateAll(deltaTime);
	}

	// 3) Update UI through IMui only
	if (auto ui = m_engine->getUiManager()) {
		ui->handleInput();
	}
}

void IApp::rigDraw() {
	// App present path first (install visuals, custom GL, etc.)
	draw();

	// Host default Draw: present ECS shapes/meshes via the main IRenderer
	// (OpenGL). Packs register Draw systems (e.g. rigSystems::SShapeRendering).
	if (auto* rendering = m_engine->getRenderingManager()) {
		if (auto main = rendering->getMainRenderer()) {
			int designW = 0, designH = 0, fbW = 0, fbH = 0;
			m_engine->getPresentSize(designW, designH, fbW, fbH);
			if (designW > 0 && designH > 0 &&
				(designW != main->getWidth() || designH != main->getHeight())) {
				main->resize(designW, designH);
			}
			main->setFramebufferSize(fbW, fbH);
			main->beginFrame();
			if (auto* ecs = m_engine->getECSManager()) {
				ecs->setPresentRenderer(main.get());
				ecs->renderSystems();
			}
			if (m_settings.graphics.showFps) {
				// Draw systems (3D especially) leave their viewport/program.
				main->beginFrame();
				presentFpsOverlay(*main, m_engine->getCurrentFrameRate(),
								  m_engine->getDeltaTime());
			}
			main->endFrame();
		}
	}

	if (auto packs = m_engine->getPackManager()) {
		packs->drawAll();
	}

	// UI pack (rigImGui) renders through IMui only - never ImGui in core.
	if (auto ui = m_engine->getUiManager()) {
		ui->render();
	}
}

void IApp::rigExit() {
	exit();
	if (auto ui = m_engine->getUiManager()) {
		ui->shutdown();
	}
}

void IApp::notifyUiAttached() {
	if (m_engine && m_engine->getECSManager()) {
		registerUIActions(m_engine->getECSManager()->getEventSystem());
	}
}

// -------------------- UI Window Management -----------------

void IApp::setWindowVisibility(const std::string& windowName, bool visible) {
	if (m_engine && m_engine->getUiManager()) {
		m_engine->getUiManager()->setWindowVisibility(windowName, visible);
	}
}

void IApp::setWindowVisibilityAll(bool visible) {
	if (m_engine && m_engine->getUiManager()) {
		m_engine->getUiManager()->setWindowVisibilityAll(visible);
	}
}

bool IApp::getWindowVisibility(const std::string& windowName) const {
	return m_engine && m_engine->getUiManager()
			   ? m_engine->getUiManager()->getWindowVisibility(windowName)
			   : false;
}

} // namespace rigkit
