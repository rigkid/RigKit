#include "rendering/U_gladGlfw.h"
#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "packs/rigAssimp/src/rigAssimp.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigJolt/src/rigJolt.h"
#include "packs/rigPhysics/src/rigPhysics.h"
#include "packs/rigPython/src/rigPython.h"
#include "packs/rigRender3D/src/rigRender3D.h"
#include "packs/rigSimComponent/src/rigSimComponent.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "CMesh.h"
#include "CTransform.h"
#include "StageBindings.h"
#include "StageProp.h"
#include "StageScene.h"

#include <cmath>
#include <spdlog/spdlog.h>
#include <string>

#if STAGE_WITH_EDITOR
#include <imgui.h>
#include <imgui_internal.h>

#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigMeshEdit/src/rigMeshEdit.h"
#endif

StageApp::StageApp() {
	window().width = 1280;
	window().height = 720;
	window().title = "Stage";
}

StageApp::~StageApp() = default;

void StageApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
	if (args.hasFlag("play")) {
		m_play = true;
	}
}

#if STAGE_WITH_EDITOR
namespace {

void dockEditor(rigkit::Mui* mui) {
	mui->setDockPassthroughCentral(true);
	mui->setGizmoOp(rigkit::IMui::GizmoOp::Translate);
	if (auto* wm = mui->getWindowManager()) {
		wm->hideWindow("Texture Paint");
	}
	mui->setDockLayoutBuilder([mui](ImGuiID dockspaceId) {
		ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
		if (!node || node->IsSplitNode()) {
			mui->setDockLayoutBuilder(nullptr);
			return;
		}
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, vp->WorkSize);
		ImGuiID right = 0, center = 0;
		ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.28f, &right, &center);
		ImGui::DockBuilderDockWindow("Mesh Edit", right);
		ImGui::DockBuilderDockWindow("Properties", right);
		ImGui::DockBuilderFinish(dockspaceId);
		mui->setDockLayoutBuilder(nullptr);
	});
}

} // namespace
#endif

bool StageApp::runSmoke() {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		spdlog::error("stage smoke: no ECS");
		return false;
	}

	const char* actors[] = {"stage-world",	"stage-ground", "stage-wind", "stage-cloth", "ball",
							"stage-camera", "stage-light",	"drop",		  "play",		 "prop"};
	for (const char* name : actors) {
		if (stage::find(*ecs, name) == entt::null) {
			spdlog::error("stage smoke: missing {}", name);
			return false;
		}
	}

	const auto& mesh = ecs->getComponent<rigkit::ecs::CMesh>(stage::find(*ecs, "prop"));
	if (mesh.positions.size() < 3) {
		spdlog::error("stage smoke: prop has {} positions", mesh.positions.size());
		return false;
	}
	if (mesh.texcoords.size() != mesh.positions.size()) {
		spdlog::error("stage smoke: prop UVs {} positions {}", mesh.texcoords.size(),
					  mesh.positions.size());
		return false;
	}
	if (stage::rigidbodyCount(*ecs) != 1) {
		spdlog::error("stage smoke: expected 1 rigidbody before the script, have {}",
					  stage::rigidbodyCount(*ecs));
		return false;
	}

	const auto cloth = stage::find(*ecs, "stage-cloth");
	const auto clothBefore = ecs->getComponent<rigkit::ecs::CMesh>(cloth).positions;
	glm::vec3 ballBefore{0.f};
	if (!stage::positionOf(*ecs, stage::find(*ecs, "ball"), ballBefore)) {
		spdlog::error("stage smoke: ball has no pose");
		return false;
	}

	if (!m_python) {
		spdlog::error("stage smoke: no Python host");
		return false;
	}
	std::string err;
	if (!stage::install(*m_python, ecs, &err)) {
		spdlog::error("stage smoke: {}", err);
		return false;
	}

	constexpr int kSteps = 120;
	constexpr float kDt = 1.f / 60.f;
	for (int i = 0; i < kSteps; ++i) {
		if (!stage::runScripts(*ecs, *m_python, kDt, &err)) {
			spdlog::error("stage smoke: {}", err);
			return false;
		}
		ecs->updateSystems(kDt);
	}

	glm::vec3 ballAfter{0.f};
	stage::positionOf(*ecs, stage::find(*ecs, "ball"), ballAfter);
	if (!(ballAfter.y < ballBefore.y - 0.5f && ballAfter.y > 0.1f && ballAfter.y < 1.2f)) {
		spdlog::error("stage smoke: ball y {:.3f} to {:.3f}", ballBefore.y, ballAfter.y);
		return false;
	}

	const auto& clothAfter = ecs->getComponent<rigkit::ecs::CMesh>(cloth).positions;
	if (clothAfter.size() != clothBefore.size() || clothAfter.empty()) {
		spdlog::error("stage smoke: cloth vertex count changed");
		return false;
	}
	float clothMove = 0.f;
	for (size_t i = 0; i < clothAfter.size(); ++i) {
		clothMove = std::max(clothMove, glm::length(clothAfter[i] - clothBefore[i]));
	}
	if (clothMove < 0.02f) {
		spdlog::error("stage smoke: cloth did not move ({:.4f})", clothMove);
		return false;
	}

	glm::vec3 marker{0.f};
	if (stage::find(*ecs, "marker") == entt::null ||
		!stage::positionOf(*ecs, stage::find(*ecs, "marker"), marker) || marker.x < 0.2f) {
		spdlog::error("stage smoke: marker missing or unmoved");
		return false;
	}
	if (stage::find(*ecs, "temp") != entt::null) {
		spdlog::error("stage smoke: temp entity survived destroy");
		return false;
	}
	if (stage::rigidbodyCount(*ecs) != 2) {
		spdlog::error("stage smoke: expected ball + spawned drop, have {} rigidbodies",
					  stage::rigidbodyCount(*ecs));
		return false;
	}

	spdlog::info("stage smoke: ball y {:.3f}, cloth move {:.3f}, marker x {:.3f}", ballAfter.y,
				 clothMove, marker.x);
	return true;
}

void StageApp::setup() {
	spdlog::info(m_smoke ? "Stage smoke" : (m_play ? "Stage play" : "Stage editor"));
	m_engine->setClearColor(0.10f, 0.11f, 0.13f, 1.f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->registerPack<rigkit::rigRender3D>();
	packs->registerPack<rigkit::rigSimComponent>();
	packs->registerPack<rigkit::rigPhysics>();
	packs->registerPack<rigkit::rigJolt>();
	packs->registerPack<rigkit::rigAssimp>();
	m_python = std::make_shared<rigkit::rigPython>();
	packs->registerPack(m_python);
#if STAGE_WITH_EDITOR
	if (!m_play && !m_smoke) {
		packs->registerPack<rigkit::rigImGui>();
		packs->registerPack<rigkit::rigMeshEdit>();
	}
#endif
	packs->initAll();
	packs->setupAll();

#if STAGE_WITH_EDITOR
	if (!m_play && !m_smoke) {
		if (auto* mui = dynamic_cast<rigkit::Mui*>(m_engine->getUiManager())) {
			dockEditor(mui);
		}
	}
#endif

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	stage::registerComponents(*ecs);
	stage::addFeatureActors(*ecs);

	std::string propError;
	const std::string propPath = AppPaths::joinPath(AppPaths::getDataDir(), "meshes/prop.gltf");
	const bool propOk = stage::addProp(*ecs, propPath, &propError);
	if (!propOk) {
		spdlog::error("Stage prop: {}", propError);
	}

	if (!m_smoke) {
		std::string err;
		if (m_python && !stage::install(*m_python, ecs, &err)) {
			spdlog::error("Stage Python: {}", err);
		}
		return;
	}

	m_smokeOk = propOk && runSmoke();
	if (m_smokeOk) {
		spdlog::info("example-stage --smoke ok");
	} else {
		spdlog::error("example-stage --smoke failed");
	}
	if (auto* win = m_engine->getWindow()) {
		glfwSetWindowShouldClose(win, GLFW_TRUE);
	}
}

void StageApp::update(float dt) {
	if (m_smoke || !m_python || !m_python->available()) {
		return;
	}
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	std::string err;
	if (!stage::runScripts(*ecs, *m_python, dt, &err)) {
		if (err != m_scriptError) {
			m_scriptError = err;
			spdlog::error("Stage script: {}", err);
		}
		return;
	}
	m_scriptError.clear();
}
