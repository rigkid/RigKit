#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/AppSettings.h"
#include "core/IApp.h"
#include "core/IMui.h"
#include "core/ISettings.h"
#include "core/WindowSettings.h"
#if !defined(_WIN32)
#include "core/util/AppIcon.h"
#endif

struct GLFWwindow;

// Forward declarations for managers
namespace rigkit {
class MEcs;
class MPack;
class MRendering;
class MCanvas;
class MSettings;
} // namespace rigkit

namespace rigkit {

/**
 * @brief Host runtime: window, managers, main loop, default OpenGL present.
 * @details Owns ECS / pack / rendering / canvas managers. Each frame clears,
 * calls IApp hooks, then runs Draw systems with the main IRenderer.
 * @see IApp, OpenGLRenderer, docs/contract/sude-loop.md
 */
class RigKitEngine : public ISettings {
  public:
	RigKitEngine(std::unique_ptr<IApp> app);
	RigKitEngine(std::unique_ptr<IApp> app, const json& settings);
	RigKitEngine(std::unique_ptr<IApp> app, const json& settings, int argc, char* argv[]);
	/**
	 * @brief Construct host runtime.
	 * @param headless If true, skip GLFW/GL present (managers + packs only).
	 *        For contract_smoke / CI - not an install present path.
	 */
	RigKitEngine(std::unique_ptr<IApp> app, const json& settings, int argc, char* argv[],
				 bool headless);
	~RigKitEngine();
	void init(std::string windowTitle = "RigKit App");
	/** @brief Blocking main loop until the window closes. No-op when headless. */
	void run();
	bool isHeadless() const { return m_headless; }

	MEcs* getECSManager() { return m_ecsManager.get(); }
	MPack* getPackManager() { return m_packManager.get(); }
	IMui* getUiManager() { return m_uiManager.get(); }
	MRendering* getRenderingManager() { return m_renderingManager.get(); }
	MCanvas* getCanvasManager() { return m_canvasManager.get(); }
	MSettings* getSettingsManager() { return m_settingsManager.get(); }

	// Graphics settings control (engine tracks frame rate)
	void setTargetFrameRate(int fps);
	int getCurrentFrameRate() const { return m_currentFps; }
	int getTargetFrameRate() const { return m_app->settings().graphics.targetFps; }
	void setVerticalSync(bool enabled);
	bool getVerticalSync() const { return m_app->settings().graphics.vsync; }
	// Frame counter (increments once per main loop iteration)
	uint64_t getFrameCount() const { return m_frameCount; }

	// Runtime window state
	int getWindowWidth() const;
	int getWindowHeight() const;
	/** @brief GLFW framebuffer width (for glReadPixels / export when HiDPI). */
	int getFramebufferWidth() const;
	/** @brief GLFW framebuffer height (for glReadPixels / export when HiDPI). */
	int getFramebufferHeight() const;
	/**
	 * @brief Present sizes for the default 2D Draw path.
	 * @details `designW/H` = logical pixels (artist / ortho). `fbW/H` =
	 * GLFW framebuffer for `glViewport`. Retina: design = window size when
	 * fb > window. Windows SCALE_TO_MONITOR: design = window / content scale
	 * when sizes match. Keeps app.json 800x600 filling the window.
	 */
	void getPresentSize(int& designW, int& designH, int& fbW, int& fbH) const;
	float getDeltaTime() const { return m_deltaTime; }

	// Background clear colour used each frame (for apps without a Canvas)
	void setClearColor(float r, float g, float b, float a = 1.0f);
	glm::vec4 getClearColor() const { return m_app->settings().graphics.clearColor; }

	GLFWwindow* getWindow() const { return m_window; }

	void attachUiManager(std::unique_ptr<IMui> ui);
	void detachUiManager();

	/** @brief Factory for an `IMui` chrome id (packs register in init). */
	using UiChromeFactory = std::function<std::unique_ptr<IMui>()>;

	/**
	 * @brief Register a chrome factory (e.g. "imgui", "tui").
	 * @details First registration for an id wins until replaced.
	 */
	void registerUiChrome(const std::string& id, UiChromeFactory factory);

	/**
	 * @brief Request a chrome swap; applied at end of frame (not mid-render).
	 * @param id Factory id ("imgui", "tui"). Empty or unknown is ignored.
	 */
	void requestUiChrome(const std::string& id);

	/** @brief Active chrome id (persisted as `ui.chrome` in settings). */
	const std::string& uiChrome() const { return m_uiChrome; }

	/** @brief Registered chrome factory ids (imgui, tui, ...). */
	std::vector<std::string> uiChromes() const;

	/**
	 * @brief Opt into the Edit Mode feature (default off). Sole source of truth.
	 * @details The UI manager reads this rather than keeping a copy, so it is
	 * safe to call before one exists. Off means panels always render (tool apps
	 * with a docked GUI). Sketches that want a clean canvas call
	 * enableEditMode(true) in setup(); panels then stay hidden until Ctrl+E.
	 */
	void enableEditMode(bool enabled);
	bool editModeEnabled() const;

	void setUiInitialised(bool v) { m_uiInitialised = v; }
	bool isUiInitialised() const { return m_uiInitialised; }

	// App debug flag (for UI packs / rigImGui menus)
	bool getDebugMode() const { return m_app && m_app->getDebugMode(); }
	void setDebugMode(bool enabled) {
		if (m_app) {
			m_app->setDebugMode(enabled);
		}
	}
	IApp* getApp() { return m_app.get(); }
	const IApp* getApp() const { return m_app.get(); }

	// Global engine access
	static RigKitEngine* getEngine() { return s_instance; }
	static void setEngine(RigKitEngine* engine) { s_instance = engine; }

	// ISettings interface
	json getSettings() const override;
	void setSettings(const json& settings) override;

	/** @brief Apply AppSettings.window to the live GLFW window (size / fullscreen). */
	void applyWindowSettingsFromApp();

	/** @brief Apply a pending chrome request (end of frame). */
	void flushPendingUiChrome();

  private:
	/** @brief Clamp design width/height to primary monitor workarea (windowed). */
	static void clampWindowedSize(int& w, int& h);

	void applyUiChrome(const std::string& id);
	void persistUiChrome() const;
	void notifyUiAttachedHooks();

	uint64_t m_frameCount = 0;
	float m_deltaTime = 0.0f;

	std::unique_ptr<IApp> m_app;
	std::unique_ptr<MEcs> m_ecsManager;
	std::unique_ptr<MPack> m_packManager;
	std::unique_ptr<IMui> m_uiManager; // UI manager provided by packs
	std::unique_ptr<MRendering> m_renderingManager;
	std::unique_ptr<MCanvas> m_canvasManager;
	std::unique_ptr<MSettings> m_settingsManager;

	GLFWwindow* m_window = nullptr;
	bool m_headless = false;

	int m_currentFps = 0;
	// Tracks whether UI manager init() has been called (pack may call it).
	bool m_uiInitialised = false;

	bool m_editModeEnabled = false;

	std::string m_uiChrome = "imgui";
	std::string m_pendingUiChrome;
	std::unordered_map<std::string, UiChromeFactory> m_uiChromeFactories;

#if !defined(_WIN32)
	/// Keeps icon RGBA alive for glfwSetWindowIcon (Linux/X11 only).
	std::vector<AppIcon::IconImage> m_windowIconImages;
#endif

	/// Windowed geometry restored when leaving fullscreen (prefs or toggle).
	bool m_haveWindowedRestore = false;
	int m_restoreX = 100;
	int m_restoreY = 100;
	int m_restoreW = 800;
	int m_restoreH = 600;

	// Global engine instance
	static RigKitEngine* s_instance;
};
} // namespace rigkit
