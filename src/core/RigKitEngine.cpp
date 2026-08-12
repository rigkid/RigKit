#include "core/RigKitEngine.h"

#include "rendering/U_gladGlfw.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <spdlog/spdlog.h>
#include <thread>

#include "core/AppSettings.h"
#include "core/IApp.h"
#include "core/TypeJson.h"
#include "core/canvas/MCanvas.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "core/util/MSettings.h"
#include "ecs/MEcs.h"
#include "rendering/MRendering.h"
#include "rendering/OpenGLRenderer.h"
#include "rendering/U_rendering.h"

#include <fstream>

namespace rigkit {

// Initialize static member
RigKitEngine* RigKitEngine::s_instance = nullptr;

RigKitEngine::~RigKitEngine() {
	if (m_settingsManager) {
		m_settingsManager->saveToDisk();
	}
	// Clear global engine instance
	s_instance = nullptr;
}

RigKitEngine::RigKitEngine(std::unique_ptr<IApp> app) : RigKitEngine(std::move(app), json{}) {}

RigKitEngine::RigKitEngine(std::unique_ptr<IApp> app, const json& settings)
	: RigKitEngine(std::move(app), settings, 0, nullptr, false) {}

RigKitEngine::RigKitEngine(std::unique_ptr<IApp> app, const json& settings, int argc, char* argv[])
	: RigKitEngine(std::move(app), settings, argc, argv, false) {}

RigKitEngine::RigKitEngine(std::unique_ptr<IApp> app, const json& settings, int argc, char* argv[],
						   bool headless)
	: m_app(std::move(app)), m_ecsManager(std::make_unique<MEcs>()),
	  m_packManager(std::make_unique<MPack>(this)), m_uiManager(nullptr),
	  m_renderingManager(std::make_unique<MRendering>()),
	  m_canvasManager(std::make_unique<MCanvas>(this)),
	  m_settingsManager(std::make_unique<MSettings>()), m_window(nullptr), m_headless(headless) {

	s_instance = this;

	// Anchor shipped data to `<exeDir>/data`; user prefs may redirect via Data Path.
	AppPaths::init(argc, argv);
	AppPaths::loadUserDataDirOverride();

	// Load preference blob early; sections apply when packs/apps register.
	m_settingsManager->loadFromDisk();

	if (m_app) {
		m_app->setEngine(this);
		// Deployed app.json next to the exe — identity + default window before CLI/prefs.
		{
			std::ifstream in(AppPaths::getManifestPath());
			if (in) {
				try {
					json manifest;
					in >> manifest;
					m_app->settings().applyFromManifest(manifest);
				} catch (const std::exception& e) {
					spdlog::warn("[RigKitEngine] Failed to read app.json: {}", e.what());
				}
			}
		}
		if (argc > 0 && argv != nullptr) {
			CommandLineArgs args(argc, argv);
			m_app->parseCommandLineArgs(args);
		}
		// Mirror OS datapath pointer into the prefs field before register (JSON
		// missing "Data Path" must not clear a surviving clean-build override).
		m_app->settings().dataPath = AppPaths::getUserDataDirOverride();
		// Register before window create so stored size/fullscreen affect this launch.
		m_settingsManager->registerPreferences(
			"host.app", "Application", &m_app->settings(), [this]() {
				if (!m_app) {
					return;
				}
				setVerticalSync(m_app->settings().graphics.vsync);
				setTargetFrameRate(m_app->settings().graphics.targetFps);
				const auto& cc = m_app->settings().graphics.clearColor;
				setClearColor(cc.r, cc.g, cc.b, cc.a);
				setDebugMode(m_app->settings().debugMode);
				applyWindowSettingsFromApp();

				const std::string before = AppPaths::getUserDataDirOverride();
				AppPaths::setUserDataDir(m_app->settings().dataPath);
				const std::string after = AppPaths::getUserDataDirOverride();
				m_app->settings().dataPath = after;
				if (before != after) {
					AppPaths::saveUserDataDirOverride();
					m_settingsManager->loadFromDisk();
					// Keep the field aligned with the OS pointer after reload.
					m_app->settings().dataPath = AppPaths::getUserDataDirOverride();
				}
			});
	}

	setSettings(settings);

	WindowSettings ws = m_app ? m_app->settings().window : WindowSettings{};

	m_ecsManager->setRenderingManager(m_renderingManager.get());

	if (m_headless) {
		spdlog::info("[RigKitEngine] headless — no GLFW/GL present");
		if (m_app) {
			m_app->rigSetup();
		}
		return;
	}

#if defined(RIGKIT_USE_ANGLE) && defined(_WIN32)
	// Must be set before glfwInit — prefer D3D11 ANGLE backend on Windows.
	glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_D3D11);
#endif
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialize GLFW");
	}

#if defined(RIGKIT_GLES)
	// Pi native GLES or desktop ANGLE — request an ES 2.0 context (Pi floor).
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#if defined(RIGKIT_USE_ANGLE)
	// ANGLE exposes GLES through EGL (desktop Windows/Linux).
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#endif
#else
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

#if defined(GLFW_SCALE_TO_MONITOR)
	// Keep window size coherent when dragged across mixed-DPI monitors (4K + 1080p).
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#endif

	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	GLFWmonitor* monitor = ws.fullscreen ? primary : nullptr;

	// app.json / prefs sizes are design (logical) pixels.
	// With GLFW_SCALE_TO_MONITOR, GLFW applies content scale — do not multiply
	// again or HiDPI launches become enormous (and prefs then save the huge size).
	int createW = ws.width > 0 ? ws.width : 800;
	int createH = ws.height > 0 ? ws.height : 600;
	float dpi = 1.f;
	if (!ws.fullscreen && primary) {
		float xscale = 1.f;
		float yscale = 1.f;
		glfwGetMonitorContentScale(primary, &xscale, &yscale);
		dpi = (xscale > yscale ? xscale : yscale);
#if !defined(GLFW_SCALE_TO_MONITOR)
		if (dpi > 1.01f) {
			createW = static_cast<int>(static_cast<float>(createW) * dpi + 0.5f);
			createH = static_cast<int>(static_cast<float>(createH) * dpi + 0.5f);
		}
#endif
		clampWindowedSize(createW, createH);
		spdlog::info("[RigKitEngine] window {}x{} (design {}x{}, dpi scale {:.2f})", createW,
					 createH, ws.width, ws.height, dpi);
	}

	m_restoreW = createW;
	m_restoreH = createH;
	m_haveWindowedRestore = !ws.fullscreen;

	m_window = glfwCreateWindow(createW, createH, ws.title.c_str(), monitor, nullptr);
	if (!m_window) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(m_window);
	// Preferences / ctor may set vsync before the window exists — apply now.
	setVerticalSync(m_app ? m_app->settings().graphics.vsync : true);
#if defined(RIGKIT_GLES)
	if (!gladLoaderLoadGLES2()) {
		throw std::runtime_error(
#if defined(RIGKIT_USE_ANGLE)
			"Failed to initialize GLAD GLES2 (ANGLE). Ensure libGLESv2/libEGL are available."
#else
			"Failed to initialize GLAD GLES2"
#endif
		);
	}
#else
	if (!gladLoaderLoadGL()) {
		throw std::runtime_error("Failed to initialize GLAD");
	}
#endif

	// Default Draw fulfillment: OpenGL IRenderer → window framebuffer.
	// Registry factory is registered by OpenGLRenderer.cpp; create + bind here
	// so creators/meshes appear without an extra Canvas/Blend2D pack.
	{
		auto main = std::make_shared<OpenGLRenderer>();
		int designW = 0, designH = 0, fbW = 0, fbH = 0;
		getPresentSize(designW, designH, fbW, fbH);
		if (designW <= 0) {
			designW = createW;
		}
		if (designH <= 0) {
			designH = createH;
		}
		if (main->initialize(designW, designH)) {
			main->setFramebufferSize(fbW > 0 ? fbW : designW, fbH > 0 ? fbH : designH);
			spdlog::info("[RigKitEngine] present design {}x{} framebuffer {}x{} (window {}x{})",
						 designW, designH, fbW > 0 ? fbW : designW, fbH > 0 ? fbH : designH,
						 getWindowWidth(), getWindowHeight());
			m_renderingManager->setMainRenderer(std::move(main));
		} else {
			spdlog::error("[RigKitEngine] Failed to initialize default OpenGL renderer");
		}
	}

	if (m_app) {
		m_app->rigSetup();
	}
}

void RigKitEngine::init(std::string windowTitle) {
	WindowSettings ws = m_app ? m_app->settings().window : WindowSettings{};
	ws.title = windowTitle;
	if (m_uiManager && !m_uiInitialised) {
		m_uiInitialised = true;
	}
}

void RigKitEngine::run() {
	if (m_headless || !m_window) {
		spdlog::warn("[RigKitEngine] run() ignored in headless mode");
		return;
	}

	using clock = std::chrono::high_resolution_clock;
	auto lastTime = clock::now();

	while (!glfwWindowShouldClose(m_window)) {
		auto frameStart = clock::now();
		m_deltaTime = std::chrono::duration<float>(frameStart - lastTime).count();
		if (m_deltaTime > 0.0f) {
			m_currentFps = static_cast<int>(1.0f / m_deltaTime);
		}
		lastTime = frameStart;

		++m_frameCount;
		m_app->rigUpdate(m_deltaTime);

		// Clear window with user-defined clear colour before custom drawing
		glm::vec4 cc = m_app->settings().graphics.clearColor;
		glClearColor(cc.r, cc.g, cc.b, cc.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_app->rigDraw();
		glfwSwapBuffers(m_window);
		glfwPollEvents();

		// Frame rate limiting (if VSync disabled or monitor faster than target)
		if (m_app->settings().graphics.targetFps > 0) {
			float targetFrame = 1.0f / static_cast<float>(m_app->settings().graphics.targetFps);
			auto frameEnd = clock::now();
			float frameDuration = std::chrono::duration<float>(frameEnd - frameStart).count();
			if (frameDuration < targetFrame) {
				std::this_thread::sleep_for(
					std::chrono::duration<float>(targetFrame - frameDuration));
			}
		}
	}

	// SUDE Exit hook
	if (m_app) {
		m_app->rigExit();
	}

	if (m_settingsManager) {
		m_settingsManager->saveToDisk();
	}

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

// -------------------- VSync & Frame Rate -----------------

void RigKitEngine::setVerticalSync(bool enabled) {
	m_app->settings().graphics.vsync = enabled;
	if (!m_window) {
		return;
	}
	glfwMakeContextCurrent(m_window);
	glfwSwapInterval(enabled ? 1 : 0);
}

void RigKitEngine::setTargetFrameRate(int fps) {
	if (fps <= 0) {
		m_app->settings().graphics.targetFps = 0; // uncapped
	} else {
		m_app->settings().graphics.targetFps = fps;
	}
}

void RigKitEngine::setClearColor(float r, float g, float b, float a) {
	m_app->settings().graphics.clearColor = glm::vec4(r, g, b, a);
}

// -------------- UI Manager attach/detach ----------------

void RigKitEngine::attachUiManager(std::unique_ptr<IMui> ui) {
	m_uiManager = std::move(ui);
	if (m_uiManager) {
		m_uiManager->setRigKitEngine(this);
	}
}

void RigKitEngine::detachUiManager() {
	m_uiManager.reset();
}

void RigKitEngine::enableEditMode(bool enabled) {
	m_editModeEnabled = enabled;
}

bool RigKitEngine::editModeEnabled() const {
	return m_editModeEnabled;
}

// -------------------- Runtime Window State -----------------

void RigKitEngine::clampWindowedSize(int& w, int& h) {
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	if (!primary) {
		return;
	}
	int mx = 0, my = 0, mw = 0, mh = 0;
	glfwGetMonitorWorkarea(primary, &mx, &my, &mw, &mh);
	if (mw <= 0 || mh <= 0) {
		return;
	}
	const int maxW = std::max(640, mw - 48);
	const int maxH = std::max(480, mh - 48);
	if (w > maxW) {
		w = maxW;
	}
	if (h > maxH) {
		h = maxH;
	}
}

void RigKitEngine::applyWindowSettingsFromApp() {
	if (m_headless || !m_window || !m_app) {
		return;
	}

	WindowSettings& ws = m_app->settings().window;
	GLFWmonitor* currentMonitor = glfwGetWindowMonitor(m_window);
	const bool isFullscreen = (currentMonitor != nullptr);
	const bool wantFullscreen = ws.fullscreen;

	if (wantFullscreen && !isFullscreen) {
		glfwGetWindowPos(m_window, &m_restoreX, &m_restoreY);
		glfwGetWindowSize(m_window, &m_restoreW, &m_restoreH);
		m_haveWindowedRestore = true;

		GLFWmonitor* primary = glfwGetPrimaryMonitor();
		if (!primary) {
			return;
		}
		const GLFWvidmode* mode = glfwGetVideoMode(primary);
		if (!mode) {
			return;
		}
		glfwSetWindowMonitor(m_window, primary, 0, 0, mode->width, mode->height, mode->refreshRate);
		return;
	}

	if (!wantFullscreen && isFullscreen) {
		int w = ws.width > 0 ? ws.width : (m_haveWindowedRestore ? m_restoreW : 800);
		int h = ws.height > 0 ? ws.height : (m_haveWindowedRestore ? m_restoreH : 600);
		clampWindowedSize(w, h);
		const int x = m_haveWindowedRestore ? m_restoreX : 100;
		const int y = m_haveWindowedRestore ? m_restoreY : 100;
		glfwSetWindowMonitor(m_window, nullptr, x, y, w, h, 0);
		ws.width = w;
		ws.height = h;
		return;
	}

	if (!wantFullscreen && !isFullscreen) {
		int w = ws.width > 0 ? ws.width : 800;
		int h = ws.height > 0 ? ws.height : 600;
		clampWindowedSize(w, h);
		// Compare logical size — GLFW window size may already be scale-multiplied.
		int designW = 0, designH = 0, fbW = 0, fbH = 0;
		getPresentSize(designW, designH, fbW, fbH);
		if (w != designW || h != designH) {
			glfwSetWindowSize(m_window, w, h);
		}
		ws.width = w;
		ws.height = h;
		m_restoreW = w;
		m_restoreH = h;
		m_haveWindowedRestore = true;
	}
}

int RigKitEngine::getWindowWidth() const {
	if (!m_window) {
		return m_app ? m_app->settings().window.width : 0;
	}
	int width, height;
	glfwGetWindowSize(m_window, &width, &height);
	return width;
}

int RigKitEngine::getWindowHeight() const {
	if (!m_window) {
		return m_app ? m_app->settings().window.height : 0;
	}
	int width, height;
	glfwGetWindowSize(m_window, &width, &height);
	return height;
}

int RigKitEngine::getFramebufferWidth() const {
	if (!m_window) {
		return getWindowWidth();
	}
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	return width;
}

int RigKitEngine::getFramebufferHeight() const {
	if (!m_window) {
		return getWindowHeight();
	}
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	return height;
}

void RigKitEngine::getPresentSize(int& designW, int& designH, int& fbW, int& fbH) const {
	fbW = getFramebufferWidth();
	fbH = getFramebufferHeight();
	const int winW = getWindowWidth();
	const int winH = getWindowHeight();

	// Retina-style: framebuffer larger than window — window size is logical.
	if (winW > 0 && winH > 0 && (fbW > winW + 1 || fbH > winH + 1)) {
		designW = winW;
		designH = winH;
		return;
	}

	// Windows-style: window size already matches framebuffer (screen pixels).
	// Divide by content scale so app.json / artist coords stay logical.
	float xscale = 1.f;
	float yscale = 1.f;
	if (m_window) {
		glfwGetWindowContentScale(m_window, &xscale, &yscale);
	}
	if (xscale < 1.01f && yscale < 1.01f) {
		if (GLFWmonitor* primary = glfwGetPrimaryMonitor()) {
			glfwGetMonitorContentScale(primary, &xscale, &yscale);
		}
	}
	if (xscale < 0.01f) {
		xscale = 1.f;
	}
	if (yscale < 0.01f) {
		yscale = 1.f;
	}
	designW = winW > 0 ? static_cast<int>(std::lround(static_cast<double>(winW) / xscale)) : 0;
	designH = winH > 0 ? static_cast<int>(std::lround(static_cast<double>(winH) / yscale)) : 0;
	if (designW <= 0) {
		designW = winW;
	}
	if (designH <= 0) {
		designH = winH;
	}
}

// -------------------- ISettings Implementation -----------------

json RigKitEngine::getSettings() const {
	json j;

	// Graphics settings
	j["graphics"]["vsync"] = getVerticalSync();
	j["graphics"]["targetFps"] = getTargetFrameRate();
	j["graphics"]["clearColor"] = colorToJson(getClearColor());

	// Window settings — logical / design pixels (not scale-multiplied GLFW size).
	int designW = 0, designH = 0, fbW = 0, fbH = 0;
	getPresentSize(designW, designH, fbW, fbH);
	j["window"]["width"] = designW > 0 ? designW : getWindowWidth();
	j["window"]["height"] = designH > 0 ? designH : getWindowHeight();

	// Engine state
	j["engine"]["currentFps"] = getCurrentFrameRate();
	j["engine"]["frameCount"] = getFrameCount();
	j["engine"]["deltaTime"] = getDeltaTime();

	return j;
}

void RigKitEngine::setSettings(const json& settings) {
	// Graphics settings
	if (settings.contains("graphics")) {
		const auto& g = settings["graphics"];
		if (g.contains("vsync")) {
			setVerticalSync(g["vsync"].get<bool>());
		}
		if (g.contains("targetFps")) {
			setTargetFrameRate(g["targetFps"].get<int>());
		}
		if (g.contains("clearColor")) {
			const glm::vec4 c = colorFromJson(g["clearColor"], getClearColor());
			setClearColor(c.r, c.g, c.b, c.a);
		}
	}

	// Window size/fullscreen live on AppSettings + applyWindowSettingsFromApp.
	// Engine state (fps, frame count, delta time) is read-only.
}

} // namespace rigkit
