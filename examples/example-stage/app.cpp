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
#include "packs/rigProject/src/ContractImport.h"
#include "packs/rigProject/src/Rigz.h"
#include "packs/rigProject/src/rigProject.h"
#include "packs/rigProjectComponent/src/rigProjectComponent.h"
#include "packs/rigPython/src/rigPython.h"
#include "packs/rigRender3D/src/rigRender3D.h"
#include "packs/rigSimComponent/src/rigSimComponent.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "CAssetRef.h"
#include "CCamera.h"
#include "CInstance.h"
#include "CLight.h"
#include "CMesh.h"
#include "CRenderMaterial.h"
#include "CRenderVisibility.h"
#include "CTransform.h"
#include "StageBindings.h"
#include "StageProp.h"
#include "StageScene.h"
#include "SurfacePass.h"

#include <cmath>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <string>

#if STAGE_WITH_EDITOR
#include <imgui.h>
#include <imgui_internal.h>

#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigMeshEdit/src/rigMeshEdit.h"
#endif

StageApp::~StageApp() {
	rigkit::project::releasePackage(m_package);
}

void StageApp::onUiAttached() {
#if STAGE_WITH_EDITOR
	m_mui = m_engine ? rigkit::authorMui(*m_engine) : nullptr;
#else
	m_mui = nullptr;
#endif
}

void StageApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
	if (args.hasFlag("play")) {
		m_play = true;
	}
	if (args.hasFlag("present")) {
		m_present = true;
		m_play = true;
	}
	if (auto open = args.getValue("open")) {
		m_openPath = *open;
	} else if (!args.getPositionalArgs().empty()) {
		const std::string& first = args.getPositionalArgs().front();
		if (first.size() > 4 &&
			(first.ends_with(".rig") || first.ends_with(".rigz") || first.ends_with(".json"))) {
			m_openPath = first;
		}
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
	const auto prop = stage::find(*ecs, "prop");
	if (!ecs->hasComponent<rigkit::ecs::CRenderMaterial>(prop)) {
		spdlog::error("stage smoke: prop has no material");
		return false;
	}
	const auto& propMat = ecs->getComponent<rigkit::ecs::CRenderMaterial>(prop);
	if (std::abs(propMat.roughness - 0.6f) > 0.02f || std::abs(propMat.albedoG - 0.75f) > 0.02f) {
		spdlog::error("stage smoke: prop material albedo {:.2f} roughness {:.2f}", propMat.albedoG,
					  propMat.roughness);
		return false;
	}
	if (propMat.albedoMap == entt::null ||
		!ecs->hasComponent<rigkit::ecs::CAssetRef>(propMat.albedoMap) ||
		ecs->getComponent<rigkit::ecs::CAssetRef>(propMat.albedoMap).path.find("checker.png") ==
			std::string::npos) {
		spdlog::error("stage smoke: prop albedo is not checker.png");
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

bool StageApp::openDocument(const std::string& path) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs || !m_document) {
		spdlog::error("stage open: missing ECS or rigProject");
		return false;
	}
	rigkit::project::releasePackage(m_package);
	m_package = rigkit::project::openPackage(path);
	if (!m_package.ok) {
		spdlog::error("stage open: {} ({})",
					  m_package.error.empty() ? "package open failed" : m_package.error, path);
		return false;
	}
	const auto result = rigkit::project::importContractFile(
		*ecs, m_package.rigPath, m_document->serializer().registry());
	if (!result.ok) {
		spdlog::error("stage open: {} ({})", result.error.empty() ? "import failed" : result.error,
					  m_package.rigPath);
		rigkit::project::releasePackage(m_package);
		return false;
	}
	for (auto entity : ecs->view<rigkit::ecs::CAssetRef>()) {
		auto& ref = ecs->getComponent<rigkit::ecs::CAssetRef>(entity);
		ref.path = rigkit::project::resolvePacketAssetPath(m_package.packetRoot, ref.path);
	}
	stage::prepareOpenedDocument(*ecs);
	spdlog::info("stage open: '{}' entities={} geometry={}", path, result.entityCount,
				 result.geometryCount);
	return true;
}

bool StageApp::runOpenSmoke() {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		spdlog::error("stage open smoke: no ECS");
		return false;
	}

	int meshCount = 0;
	entt::entity meshEntity = entt::null;
	for (auto entity : ecs->view<rigkit::ecs::CMesh>()) {
		++meshCount;
		if (meshEntity == entt::null) {
			meshEntity = entity;
		}
	}
	if (meshCount < 1 || meshEntity == entt::null) {
		spdlog::error("stage open smoke: expected a mesh");
		return false;
	}
	const auto& mesh = ecs->getComponent<rigkit::ecs::CMesh>(meshEntity);
	if (mesh.positions.size() < 3) {
		spdlog::error("stage open smoke: mesh has {} positions", mesh.positions.size());
		return false;
	}
	if (!ecs->hasComponent<rigkit::ecs::CRenderMaterial>(meshEntity)) {
		spdlog::error("stage open smoke: mesh has no material");
		return false;
	}

	int activeCameras = 0;
	for (auto entity : ecs->view<rigkit::ecs::CCamera>()) {
		if (ecs->getComponent<rigkit::ecs::CCamera>(entity).active) {
			++activeCameras;
		}
	}
	if (activeCameras < 1) {
		spdlog::error("stage open smoke: no active camera");
		return false;
	}

	int enabledLights = 0;
	int enabledDirectional = 0;
	int enabledPoint = 0;
	entt::entity keptLight = entt::null;
	for (auto entity : ecs->view<rigkit::ecs::CLight, rigkit::ecs::CTransform>()) {
		const auto& light = ecs->getComponent<rigkit::ecs::CLight>(entity);
		if (!light.enabled) {
			continue;
		}
		++enabledLights;
		keptLight = entity;
		if (light.type == rigkit::ecs::CLight::Type::Directional) {
			++enabledDirectional;
		} else if (light.type == rigkit::ecs::CLight::Type::Point) {
			++enabledPoint;
		}
	}
	if (enabledLights != 1 || keptLight == entt::null) {
		spdlog::error("stage open smoke: expected one enabled light (have {})", enabledLights);
		return false;
	}
	if (enabledDirectional == 1) {
		const auto& sunXf = ecs->getComponent<rigkit::ecs::CTransform>(keptLight);
		if (std::fabs(sunXf.rotation.x - (-0.4226f)) > 0.01f ||
			std::fabs(sunXf.rotation.w - 0.9063f) > 0.01f) {
			spdlog::error("stage open smoke: sun rotation stomped ({:.4f}, {:.4f}, {:.4f}, {:.4f})",
						  sunXf.rotation.x, sunXf.rotation.y, sunXf.rotation.z, sunXf.rotation.w);
			return false;
		}
	} else if (enabledPoint != 1) {
		spdlog::error("stage open smoke: expected directional or point light");
		return false;
	}

	int instanceCount = 0;
	for (auto entity : ecs->view<rigkit::ecs::CInstance>()) {
		const auto source = ecs->getComponent<rigkit::ecs::CInstance>(entity).source;
		if (!ecs->registry().valid(source) || !ecs->hasComponent<rigkit::ecs::CMesh>(source)) {
			spdlog::error("stage open smoke: instance source missing mesh");
			return false;
		}
		++instanceCount;
	}

	int hiddenCount = 0;
	for (auto entity : ecs->view<rigkit::ecs::CRenderVisibility>()) {
		if (!ecs->getComponent<rigkit::ecs::CRenderVisibility>(entity).visible) {
			++hiddenCount;
		}
	}

	const auto& material = ecs->getComponent<rigkit::ecs::CRenderMaterial>(meshEntity);
	if (material.albedoMap != entt::null) {
		if (!ecs->registry().valid(material.albedoMap) ||
			!ecs->hasComponent<rigkit::ecs::CAssetRef>(material.albedoMap)) {
			spdlog::error("stage open smoke: albedoMap entity missing");
			return false;
		}
		const auto& href = ecs->getComponent<rigkit::ecs::CAssetRef>(material.albedoMap).path;
		if (href.empty() || !std::filesystem::exists(href)) {
			spdlog::error("stage open smoke: albedo file missing ({})", href);
			return false;
		}
	}

	spdlog::info("stage open smoke: mesh verts={} cameras={} light={} instances={} hidden={} "
				 "albedo={}",
				 mesh.positions.size(), activeCameras,
				 enabledDirectional ? "directional" : "point", instanceCount, hiddenCount,
				 material.albedoMap != entt::null);

	// Phase 2: textured package also round-trips through a temp .rigz.
	if (m_openPath.find("textured") != std::string::npos && !m_package.temporary) {
		const std::string dest =
			AppPaths::joinPath(AppPaths::getExecutableDir(), "data/scenes/textured-smoke.rigz");
		std::string err;
		if (!rigkit::project::writeRigz(dest, m_package.rigPath, m_package.packetRoot, &err)) {
			spdlog::error("stage open smoke: writeRigz failed ({})", err);
			return false;
		}
		if (!openDocument(dest)) {
			spdlog::error("stage open smoke: reopen .rigz failed");
			return false;
		}
		bool albedoOk = false;
		for (auto entity : ecs->view<rigkit::ecs::CRenderMaterial>()) {
			const auto& mat = ecs->getComponent<rigkit::ecs::CRenderMaterial>(entity);
			if (mat.albedoMap == entt::null ||
				!ecs->hasComponent<rigkit::ecs::CAssetRef>(mat.albedoMap)) {
				continue;
			}
			const auto& href = ecs->getComponent<rigkit::ecs::CAssetRef>(mat.albedoMap).path;
			if (!href.empty() && std::filesystem::exists(href)) {
				albedoOk = true;
				break;
			}
		}
		if (!albedoOk) {
			spdlog::error("stage open smoke: .rigz albedo path missing after reopen");
			return false;
		}
		spdlog::info("stage open smoke: .rigz round-trip ok");
	}
	return true;
}

void StageApp::setup() {
	const bool opening = !m_openPath.empty();
	spdlog::info(opening	 ? "Stage open"
				 : m_smoke	 ? "Stage smoke"
				 : m_play	 ? "Stage play"
							 : "Stage editor");
	m_engine->setClearColor(0.10f, 0.11f, 0.13f, 1.f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigProjectComponent>();
	packs->registerPack<rigkit::rigProject>();
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
	m_document = packs->getPack<rigkit::rigProject>();

#if STAGE_WITH_EDITOR
	if (!m_play && !m_smoke) {
		m_mui = m_engine ? rigkit::authorMui(*m_engine) : nullptr;
		if (m_mui) {
			dockEditor(m_mui);
		}
	}
#endif

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	stage::registerComponents(*ecs);

	if (opening) {
		namespace fs = std::filesystem;
		fs::path path(m_openPath);
		if (!path.is_absolute()) {
			const fs::path besideExe = fs::path(AppPaths::getExecutableDir()) / m_openPath;
			const fs::path inData = fs::path(AppPaths::getDataDir()) / m_openPath;
			const fs::path inScenes =
				fs::path(AppPaths::getDataDir()) / "scenes" / path.filename();
			if (fs::exists(besideExe)) {
				path = besideExe;
			} else if (fs::exists(inData)) {
				path = inData;
			} else if (fs::exists(inScenes)) {
				path = inScenes;
			} else {
				path = besideExe;
			}
		}
		const bool opened = openDocument(path.string());
		if (m_smoke) {
			m_smokeOk = opened && runOpenSmoke();
			if (m_smokeOk) {
				spdlog::info("example-stage --open --smoke ok");
			} else {
				spdlog::error("example-stage --open --smoke failed");
			}
			if (auto* win = m_engine->getWindow()) {
				glfwSetWindowShouldClose(win, GLFW_TRUE);
			}
			return;
		}
		if (!opened) {
			m_smokeOk = false;
		}
		return;
	}

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
	if (m_present) {
		if (++m_presentFrames >= 3) {
			m_presentOk = !rigkit::ecs::surfacePresentFailed();
			if (!m_presentOk) {
				spdlog::error("stage present: surface shader or asset failed");
			} else {
				spdlog::info("stage present: surface ok");
			}
			if (auto* win = m_engine->getWindow()) {
				glfwSetWindowShouldClose(win, GLFW_TRUE);
			}
		}
	}
	if (m_smoke || !m_openPath.empty() || !m_python || !m_python->available()) {
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
