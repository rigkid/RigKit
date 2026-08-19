#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "core/ISettings.h"
#include "core/json.h"
#include "core/TypeJson.h"
#include "core/WindowSettings.h"
#include "ecs/PropertyReflection.h"

namespace rigkit {

struct GraphicsSettings {
	bool vsync = true;
	int targetFps = 60; // 0 = uncapped (but still limited by vsync)
	glm::vec4 clearColor{0.4f, 0.4f, 0.4f, 1.0f};
};

struct AppSettings : public ISettings {
	std::string appName = "RigKit App";
	/// Semver / display string from `app.json` `"version"` (e.g. `"0.1.0"`).
	std::string version = "0.1.0";
	std::string description;
	/// SPDX id from `app.json` `"license"` (required by CI / check-invariants).
	std::string license;

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

	/**
	 * @brief Apply identity fields from a deployed `app.json` (next to the exe).
	 * @details Reads `name`, `version`, `description`, `license`, and optional
	 * `window`. Does not touch graphics / debug / dataPath.
	 */
	void applyFromManifest(const json& j) {
		if (j.contains("name") && j["name"].is_string()) {
			appName = j["name"].get<std::string>();
		}
		if (j.contains("version")) {
			if (j["version"].is_string()) {
				version = j["version"].get<std::string>();
			} else if (j["version"].is_number()) {
				version = std::to_string(j["version"].get<double>());
			}
		}
		if (j.contains("description") && j["description"].is_string()) {
			description = j["description"].get<std::string>();
		}
		if (j.contains("license") && j["license"].is_string()) {
			license = j["license"].get<std::string>();
		}
		if (j.contains("window") && j["window"].is_object()) {
			const auto& w = j["window"];
			if (w.contains("width"))
				window.width = w["width"].get<int>();
			if (w.contains("height"))
				window.height = w["height"].get<int>();
			if (w.contains("title") && w["title"].is_string())
				window.title = w["title"].get<std::string>();
			if (w.contains("fullscreen"))
				window.fullscreen = w["fullscreen"].get<bool>();
			if (w.contains("samples"))
				window.samples = w["samples"].get<int>();
		}
	}

	// ISettings implementation (JSON serialisation)
	json getSettings() const override {
		json j;
		j["appName"] = appName;
		j["version"] = version;
		j["description"] = description;
		j["license"] = license;
		j["debugMode"] = debugMode;
		// Window
		j["window"]["width"] = window.width;
		j["window"]["height"] = window.height;
		j["window"]["title"] = window.title;
		j["window"]["fullscreen"] = window.fullscreen;
		j["window"]["samples"] = window.samples;
		// Graphics
		j["graphics"]["vsync"] = graphics.vsync;
		j["graphics"]["targetFps"] = graphics.targetFps;
		j["graphics"]["clearColor"] = colorToJson(graphics.clearColor);
		return j;
	}

	void setSettings(const json& j) override {
		if (j.contains("appName"))
			appName = j["appName"].get<std::string>();
		if (j.contains("version")) {
			if (j["version"].is_string())
				version = j["version"].get<std::string>();
			else if (j["version"].is_number())
				version = std::to_string(j["version"].get<double>());
		}
		if (j.contains("description") && j["description"].is_string())
			description = j["description"].get<std::string>();
		if (j.contains("license") && j["license"].is_string())
			license = j["license"].get<std::string>();
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
			if (w.contains("samples"))
				window.samples = w["samples"].get<int>();
		}

		if (j.contains("graphics")) {
			const auto& g = j["graphics"];
			if (g.contains("vsync"))
				graphics.vsync = g["vsync"].get<bool>();
			if (g.contains("targetFps"))
				graphics.targetFps = g["targetFps"].get<int>();
			if (g.contains("clearColor")) {
				graphics.clearColor = colorFromJson(g["clearColor"], graphics.clearColor);
			}
		}
	}
};

} // namespace rigkit
