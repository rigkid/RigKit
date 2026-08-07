#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "core/ISettings.h"
#include "core/WindowSettings.h"
#include "core/json.h"
#include "ecs/PropertyReflection.h"

namespace rigkit {

struct GraphicsSettings {
	bool vsync = true;
	int targetFps = 60; // 0 = uncapped (but still limited by vsync)
	glm::vec4 clearColor{0.4f, 0.4f, 0.4f, 1.0f};
};

struct AppSettings : public ISettings {
	std::string appName = "RigKit App";
	float version = 0.1f;

	WindowSettings window; // default constructed (800x600, etc.)
	GraphicsSettings graphics;
	bool debugMode = false;
	/// User data root override (empty = `<exeDir>/data`). Preferences / workspaces / themes.
	std::string dataPath;

	/// Preference / inspector fields (window size/fullscreen apply immediately).
	std::vector<sProp> GetProperties() {
		return {
			{0, "Debug Mode", EPT_BOOL, &debugMode},
			{1, "VSync", EPT_BOOL, &graphics.vsync},
			{2, "Target FPS", EPT_INT, &graphics.targetFps},
			{3, "Clear Color", EPT_COLOR, &graphics.clearColor},
			{4, "Window Width", EPT_INT, &window.width},
			{5, "Window Height", EPT_INT, &window.height},
			{6, "Fullscreen", EPT_BOOL, &window.fullscreen},
			{7, "Data Path", EPT_STRING, &dataPath},
		};
	}

	// ISettings implementation (JSON serialisation)
	json getSettings() const override {
		json j;
		j["appName"] = appName;
		j["version"] = version;
		j["debugMode"] = debugMode;
		// Window
		j["window"]["width"] = window.width;
		j["window"]["height"] = window.height;
		j["window"]["title"] = window.title;
		j["window"]["fullscreen"] = window.fullscreen;
		// Graphics
		j["graphics"]["vsync"] = graphics.vsync;
		j["graphics"]["targetFps"] = graphics.targetFps;
		j["graphics"]["clearColor"] = {graphics.clearColor.r, graphics.clearColor.g,
									   graphics.clearColor.b, graphics.clearColor.a};
		return j;
	}

	void setSettings(const json& j) override {
		if (j.contains("appName"))
			appName = j["appName"].get<std::string>();
		if (j.contains("version"))
			version = j["version"].get<float>();
		if (j.contains("debugMode"))
			debugMode = j["debugMode"].get<bool>();

		if (j.contains("window")) {
			const auto& w = j["window"];
			if (w.contains("width"))
				window.width = w["width"].get<int>();
			if (w.contains("height"))
				window.height = w["height"].get<int>();
			if (w.contains("title"))
				window.title = w["title"].get<std::string>();
			if (w.contains("fullscreen"))
				window.fullscreen = w["fullscreen"].get<bool>();
		}

		if (j.contains("graphics")) {
			const auto& g = j["graphics"];
			if (g.contains("vsync"))
				graphics.vsync = g["vsync"].get<bool>();
			if (g.contains("targetFps"))
				graphics.targetFps = g["targetFps"].get<int>();
			if (g.contains("clearColor")) {
				auto c = g["clearColor"];
				if (c.is_array() && c.size() == 4) {
					graphics.clearColor = glm::vec4(c[0].get<float>(), c[1].get<float>(),
													c[2].get<float>(), c[3].get<float>());
				}
			}
		}
	}
};

} // namespace rigkit
