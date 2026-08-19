#pragma once

#include "core/AppSettings.h"
#include "core/util/CommandLineArgs.h"
#include "core/WindowSettings.h"

namespace rigkit {

class RigKitEngine;

namespace ecs {
class SEvent;
}

/**
 * @brief Application entry surface — setup / update / draw / exit.
 * @details Override setup/update/draw/exit. Do not override rigSetup/rigUpdate/
 * rigDraw/rigExit — those are host template methods (SUDE + present).
 * @see RigKitEngine, docs/contract/sude-loop.md, docs/authoring.md
 */
class IApp {
  public:
	virtual ~IApp() = default;

	/** @brief Host SUDE glue — apps must not override. */
	void rigSetup();
	void rigUpdate(float deltaTime);
	void rigDraw();
	void rigExit();

	void setEngine(rigkit::RigKitEngine* engine) { m_engine = engine; }
	rigkit::RigKitEngine* getEngine() const { return m_engine; }

	AppSettings& settings() { return m_settings; }
	const AppSettings& settings() const { return m_settings; }

	WindowSettings& window() { return m_settings.window; }
	const WindowSettings& window() const { return m_settings.window; }

	void setDebugMode(bool enabled) { m_settings.debugMode = enabled; }
	bool getDebugMode() const { return m_settings.debugMode; }
	void toggleDebugMode() { m_settings.debugMode = !m_settings.debugMode; }

	/**
	 * @brief Parse common CLI flags (--debug, --app-id, --window-size WxH).
	 * @note --config is reserved; file load not wired yet.
	 */
	virtual void parseCommandLineArgs(const CommandLineArgs& args) {
		if (args.hasFlag("debug")) {
			setDebugMode(true);
		}

		if (auto configFile = args.getValue("config")) {
			(void)configFile;
		}

		if (auto appId = args.getValue("app-id")) {
			m_settings.appName = appId.value();
		}

		if (auto windowSize = args.getValue("window-size")) {
			size_t xPos = windowSize.value().find('x');
			if (xPos != std::string::npos) {
				try {
					int width = std::stoi(windowSize.value().substr(0, xPos));
					int height = std::stoi(windowSize.value().substr(xPos + 1));
					m_settings.window.width = width;
					m_settings.window.height = height;
				} catch (...) {
				}
			}
		}
	}

	const std::string& getAppName() const { return m_settings.appName; }
	const std::string& getAppVersion() const { return m_settings.version; }
	const std::string& getAppDescription() const { return m_settings.description; }
	const std::string& getAppLicense() const { return m_settings.license; }

	void setWindowVisibility(const std::string& windowName, bool visible);
	void setWindowVisibilityAll(bool visible);
	bool getWindowVisibility(const std::string& windowName) const;

	/**
	 * @brief Re-bind app UI actions after an `IMui` chrome swap.
	 * @details Calls registerUIActions when ECS is available. Packs also get
	 * IPack::onUiAttached from the engine.
	 */
	void notifyUiAttached();

  protected:
	/** @brief User hooks — override these (see sude-loop.md). */
	virtual void setup() {}
	virtual void update(float) {}
	virtual void draw() {}
	virtual void exit() {}

	virtual void registerUIActions(rigkit::ecs::SEvent&) {}

	rigkit::RigKitEngine* m_engine = nullptr;
	AppSettings m_settings;
};
} // namespace rigkit
