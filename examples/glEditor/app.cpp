#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "ecs/MEcs.h"
#include "packs/rigCodeEditor/src/rigCodeEditor.h"
#include "packs/rigComponent/src/CAssetRef.h"
#include "packs/rigComponent/src/CCode.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigImGui/src/MWindow.h"
#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "ShaderPreviewWindow.h"

#include <imgui.h>
#include <fstream>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace {

std::string readFileOrEmpty(const std::string& path) {
	std::ifstream in(path);
	if (!in) {
		return {};
	}
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

} // namespace

GlEditorApp::GlEditorApp() {
	window().width = 1200;
	window().height = 720;
	window().title = "glEditor";
}

void GlEditorApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
}

bool GlEditorApp::seedShader(const std::string& name, const std::string& relativePath, int order) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return false;
	}
	const std::string path = AppPaths::joinPath(AppPaths::getDataDir(), relativePath);
	std::string text = readFileOrEmpty(path);
	if (text.empty()) {
		spdlog::warn("glEditor - missing {}", path);
		return false;
	}

	const auto e = ecs->createEntity();
	rigkit::ecs::CCode code;
	code.name = name;
	code.text = std::move(text);
	code.language = "glsl";
	code.order = order;
	code.dirty = false;
	code.epoch = 1;
	ecs->addComponent<rigkit::ecs::CCode>(e, std::move(code));

	rigkit::ecs::CAssetRef asset;
	asset.kind = rigkit::ecs::CAssetRef::Kind::Other;
	asset.path = path;
	ecs->addComponent<rigkit::ecs::CAssetRef>(e, std::move(asset));

	std::error_code ec;
	const auto mtime = std::filesystem::last_write_time(path, ec);
	if (!ec) {
		m_mtimes[path] = mtime;
	}
	return true;
}

void GlEditorApp::watchDiskBuffers() {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	for (auto e : ecs->registry().view<rigkit::ecs::CCode>()) {
		auto& code = ecs->getComponent<rigkit::ecs::CCode>(e);
		if (!ecs->hasComponent<rigkit::ecs::CAssetRef>(e)) {
			continue;
		}
		const auto& asset = ecs->getComponent<rigkit::ecs::CAssetRef>(e);
		if (asset.path.empty()) {
			continue;
		}
		std::error_code ec;
		const auto mtime = std::filesystem::last_write_time(asset.path, ec);
		if (ec) {
			continue;
		}
		auto it = m_mtimes.find(asset.path);
		if (it != m_mtimes.end() && it->second == mtime) {
			continue;
		}
		m_mtimes[asset.path] = mtime;
		std::string text = readFileOrEmpty(asset.path);
		if (text.empty() || text == code.text) {
			continue;
		}
		code.text = std::move(text);
		code.dirty = false;
		code.epoch += 1;
		spdlog::info("glEditor - disk reload {}", asset.path);
	}
}

bool GlEditorApp::runSmoke() {
	if (!m_previewWin) {
		spdlog::error("glEditor smoke: no preview window");
		return false;
	}
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return false;
	}
	entt::entity target = entt::null;
	for (auto e : ecs->registry().view<rigkit::ecs::CCode>()) {
		const auto& c = ecs->getComponent<rigkit::ecs::CCode>(e);
		if (c.language == "glsl" && c.name == "gradient") {
			target = e;
			break;
		}
	}
	if (target == entt::null) {
		spdlog::error("glEditor smoke: no gradient CCode");
		return false;
	}
	m_previewWin->setPreviewEntity(target);
	if (!m_previewWin->compileNow()) {
		spdlog::error("glEditor smoke: compile failed: {}", m_previewWin->preview().error());
		return false;
	}
	if (!m_previewWin->preview().hasProgram()) {
		spdlog::error("glEditor smoke: no program after compile");
		return false;
	}
	// One present pass to exercise FBO.
	m_previewWin->preview().render(64, 64, 0.f, {0.f, 0.f, 0.f, 0.f});
	return true;
}

void GlEditorApp::setup() {
	spdlog::info("glEditor - Shadertoy-style GLES preview over CCode");
	m_engine->setClearColor(0.07f, 0.07f, 0.086f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}

	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->registerPack<rigkit::rigImGui>();

	rigkit::rigCodeEditor::Options opts;
	opts.registerWindow = true;
	opts.windowVisible = true;
	opts.fontSize = 14.f;
	packs->registerPack<rigkit::rigCodeEditor>(opts);

	packs->initAll();
	packs->setupAll();

	seedShader("gradient", "gradient.glsl", 0);
	seedShader("plasma", "plasma.glsl", 1);

	auto* ui = m_engine->getUiManager();
	if (ui && ui->getWindowManager()) {
		auto win = ui->getWindowManager()->createWindow<ShaderPreviewWindow>();
		m_previewWin = win.get();
		ui->getWindowManager()->showWindow("Shader Preview");
		ui->getWindowManager()->showWindow("Code Editor");

		if (auto* mui = dynamic_cast<rigkit::Mui*>(ui)) {
			// First-run: preview fills the central dock; code editor on the right.
			// No-ops once imgui.ini has restored a split.
			mui->setDockLayoutBuilder([mui](ImGuiID dockspaceId) {
				ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
				if (!node) {
					return;
				}
				if (node->IsSplitNode()) {
					mui->setDockLayoutBuilder(nullptr);
					return;
				}
				const ImGuiViewport* vp = ImGui::GetMainViewport();
				ImGui::DockBuilderRemoveNode(dockspaceId);
				ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
				ImGui::DockBuilderSetNodeSize(dockspaceId, vp->WorkSize);

				ImGuiID right = 0, center = 0;
				ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.38f, &right, &center);
				ImGui::DockBuilderDockWindow("Shader Preview", center);
				ImGui::DockBuilderDockWindow("Code Editor", right);
				ImGui::DockBuilderFinish(dockspaceId);
				mui->setDockLayoutBuilder(nullptr);
			});
		}

		// Prefer gradient as the first preview target.
		if (auto* ecs = m_engine->getECSManager()) {
			for (auto e : ecs->registry().view<rigkit::ecs::CCode>()) {
				if (ecs->getComponent<rigkit::ecs::CCode>(e).name == "gradient") {
					m_previewWin->setPreviewEntity(e);
					m_previewWin->compileNow();
					break;
				}
			}
		}
	}

	if (m_smoke) {
		m_smokeOk = runSmoke();
		if (!m_smokeOk) {
			spdlog::error("glEditor --smoke failed");
		} else {
			spdlog::info("glEditor --smoke ok");
		}
		if (auto* win = m_engine->getWindow()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
	}
}

void GlEditorApp::update(float dt) {
	watchDiskBuffers();
	if (m_previewWin) {
		m_previewWin->tick(dt);
	}
}
