#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "ecs/MEcs.h"
#include "packs/rigCodeEditor/src/CodeEditorWindow.h"
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
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

namespace {

/**
 * @brief One seeded example: display name (learning order) + file in `data/`.
 * @details The number in the file name matches the order so the Code Editor
 * tabs, the preview Buffer list, and the files on disk all read the same way.
 */
struct ExampleShader {
	const char* name;
	const char* file;
};

constexpr ExampleShader kExamples[] = {
	{"01 First Colour", "01-first-colour.glsl"},
	{"02 Mixing Two Colours", "02-mixing-two-colours.glsl"},
	{"03 Rainbow Gradient", "03-rainbow-gradient.glsl"},
	{"04 Stripes", "04-stripes.glsl"},
	{"05 A Soft Circle", "05-soft-circle.glsl"},
	{"06 Bouncing Ball", "06-bouncing-ball.glsl"},
	{"07 Follow the Mouse", "07-follow-the-mouse.glsl"},
	{"08 Checkerboard", "08-checkerboard.glsl"},
	{"09 Target Rings", "09-target-rings.glsl"},
	{"10 Plasma", "10-plasma.glsl"},
	{"11 Polka Dots", "11-polka-dots.glsl"},
	{"12 Sunset", "12-sunset.glsl"},
	{"13 Wavy Picture", "13-wavy-picture.glsl"},
	{"14 Twinkling Stars", "14-twinkling-stars.glsl"},
	{"15 Clouds", "15-clouds.glsl"},
	{"16 Tunnel", "16-tunnel.glsl"},
	{"17 Kaleidoscope", "17-kaleidoscope.glsl"},
	{"18 Julia Fractal", "18-julia-fractal.glsl"},
	{"99 Make Your Own", "99-make-your-own.glsl"},
};

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

void GlEditorApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
}

entt::entity GlEditorApp::seedShader(const std::string& name, const std::string& relativePath,
									 int order) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return entt::null;
	}
	const std::string path = AppPaths::joinPath(AppPaths::getDataDir(), relativePath);
	std::string text = readFileOrEmpty(path);
	if (text.empty()) {
		spdlog::warn("glEditor - missing {}", path);
		return entt::null;
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
	return e;
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

void GlEditorApp::onUiAttached() {
	m_mui = m_engine ? rigkit::authorMui(*m_engine) : nullptr;
}

void GlEditorApp::syncPreviewWithEditor() {
	if (!m_previewWin || !m_mui) {
		return;
	}
	auto* wm = m_mui->getWindowManager();
	if (!wm) {
		return;
	}
	auto editorWin = wm->getWindow<rigkit::CodeEditorWindow>("Code Editor");
	if (!editorWin) {
		return;
	}
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}

	// Editor tab changed (user clicked a tab): preview follows, so the
	// shader being edited is always the shader on screen.
	const std::string editorId = editorWin->panel().selectedSidebarId();
	if (!editorId.empty() && editorId != m_syncedEditorBuffer) {
		m_syncedEditorBuffer = editorId;
		entt::entity picked = entt::null;
		try {
			picked = static_cast<entt::entity>(std::stoul(editorId));
		} catch (...) {
			picked = entt::null;
		}
		if (picked != entt::null && ecs->registry().valid(picked) &&
			ecs->hasComponent<rigkit::ecs::CCode>(picked) &&
			ecs->getComponent<rigkit::ecs::CCode>(picked).language == "glsl" &&
			picked != m_previewWin->previewEntity()) {
			m_previewWin->setPreviewEntity(picked);
			m_syncedPreview = picked;
			return;
		}
	}

	// Preview Buffer changed: editor follows, so there is one "current
	// shader" in the whole app, not one per window.
	const entt::entity previewEntity = m_previewWin->previewEntity();
	if (previewEntity != entt::null && previewEntity != m_syncedPreview) {
		m_syncedPreview = previewEntity;
		const std::string id = std::to_string(static_cast<uint32_t>(previewEntity));
		if (editorWin->panel().selectSidebarEntryById(id)) {
			m_syncedEditorBuffer = id;
		}
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

	// Every seeded example must compile. A broken example is a broken
	// first impression, so it fails the smoke like any other defect.
	std::vector<entt::entity> buffers;
	for (auto e : ecs->registry().view<rigkit::ecs::CCode>()) {
		if (ecs->getComponent<rigkit::ecs::CCode>(e).language == "glsl") {
			buffers.push_back(e);
		}
	}
	std::sort(buffers.begin(), buffers.end(), [ecs](entt::entity a, entt::entity b) {
		return ecs->getComponent<rigkit::ecs::CCode>(a).order <
			   ecs->getComponent<rigkit::ecs::CCode>(b).order;
	});
	if (buffers.empty()) {
		spdlog::error("glEditor smoke: no glsl CCode buffers seeded");
		return false;
	}

	bool ok = true;
	for (auto e : buffers) {
		const std::string name = ecs->getComponent<rigkit::ecs::CCode>(e).name;
		m_previewWin->setPreviewEntity(e);
		if (!m_previewWin->compileNow() || !m_previewWin->preview().hasProgram()) {
			spdlog::error("glEditor smoke: '{}' failed to compile: {}", name,
						  m_previewWin->preview().error());
			ok = false;
		} else {
			spdlog::info("glEditor smoke: '{}' ok", name);
		}
	}
	if (!ok) {
		return false;
	}

	// One present pass to exercise FBO.
	m_previewWin->preview().render(64, 64, 0.f, {0.f, 0.f, 0.f, 0.f});

	// A typo must not blank the picture: a failing compile keeps the
	// last-good program and reports the error.
	const std::string broken = "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
							   "\tfragColor = vec4(1.0)\n" // missing ; on purpose
							   "}\n";
	if (m_previewWin->preview().compile(broken)) {
		spdlog::error("glEditor smoke: broken shader compiled (test itself is wrong)");
		return false;
	}
	if (!m_previewWin->preview().hasProgram() || !m_previewWin->preview().hasError()) {
		spdlog::error("glEditor smoke: broken shader lost the last-good program or the error");
		return false;
	}
	spdlog::info("glEditor smoke: broken shader kept last-good, error was: {}",
				 m_previewWin->preview().error());

	// Leave the first example compiled, as setup found it.
	m_previewWin->setPreviewEntity(buffers.front());
	return m_previewWin->compileNow();
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

	entt::entity firstShader = entt::null;
	int order = 0;
	for (const auto& example : kExamples) {
		const entt::entity e = seedShader(example.name, example.file, order++);
		if (firstShader == entt::null) {
			firstShader = e;
		}
	}

	m_mui = m_engine ? rigkit::authorMui(*m_engine) : nullptr;
	auto* wm = m_mui ? m_mui->getWindowManager() : nullptr;
	if (m_mui && wm) {
		auto win = wm->createWindow<ShaderPreviewWindow>();
		m_previewWin = win.get();
		wm->showWindow("Shader Preview");
		wm->showWindow("Code Editor");

		// First-run: preview fills the central dock; code editor on the right.
		// No-ops once imgui.ini has restored a split.
		m_mui->setDockLayoutBuilder([this](ImGuiID dockspaceId) {
			if (!m_mui) {
				return;
			}
			ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
			if (!node) {
				return;
			}
			if (node->IsSplitNode()) {
				m_mui->setDockLayoutBuilder(nullptr);
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
			m_mui->setDockLayoutBuilder(nullptr);
		});

		// Start on the first example; sync keeps the editor tab with it.
		if (firstShader != entt::null) {
			m_previewWin->setPreviewEntity(firstShader);
			m_previewWin->compileNow();
			m_syncedPreview = firstShader;
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
	syncPreviewWithEditor();
	if (m_previewWin) {
		m_previewWin->tick(dt);
	}
}
