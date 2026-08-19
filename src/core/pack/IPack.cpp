#include "IPack.h"

#include "MPack.h"
#include "RigKitEngine.h"

#include <iostream>
#include <spdlog/spdlog.h>

namespace rigkit {

IPack::IPack(const std::string& name)
	: m_name(name), m_description(""), m_license("MIT Rigkid Contributors"), m_url(""),
	  m_version(""), m_enabled(true), m_initialized(false), m_time(0.0f) {}

// Destructor is defaulted in header

void IPack::addDependency(const std::string& packName) {
	m_dependencies.push_back(packName);
}

void IPack::setDependencies(std::vector<std::string> deps) {
	m_dependencies = std::move(deps);
}

void IPack::addEventListener(const std::string& event, std::function<void()> callback) {
	m_eventListeners[event].push_back(callback);
}

void IPack::removeEventListener(const std::string& event) {
	auto it = m_eventListeners.find(event);
	if (it != m_eventListeners.end()) {
		m_eventListeners.erase(it);
	}
}

void IPack::triggerEvent(const std::string& event) {
	auto it = m_eventListeners.find(event);
	if (it != m_eventListeners.end()) {
		for (auto& callback : it->second) {
			callback();
		}
	}
}

void IPack::log(const std::string& message) {
	spdlog::info("[Pack {}] {}", m_name, message);
}

void IPack::error(const std::string& message) {
	spdlog::error("[Pack {} ERROR] {}", m_name, message);
}

void IPack::warning(const std::string& message) {
	spdlog::warn("[Pack {} WARNING] {}", m_name, message);
}

// Parameter system implementation
void IPack::setParameter(const std::string& name, float value) {
	m_parameters[name] = value;

	// Trigger callback if registered
	auto it = m_parameterCallbacks.find(name);
	if (it != m_parameterCallbacks.end()) {
		it->second(value);
	}

	this->log("Parameter " + name + " set to " + std::to_string(value));
}

float IPack::getParameter(const std::string& name) const {
	auto it = m_parameters.find(name);
	if (it != m_parameters.end()) {
		return it->second;
	}
	return 0.0f;
}

void IPack::onParameterChanged(const std::string& name, std::function<void(float)> callback) {
	m_parameterCallbacks[name] = callback;
}

void IPack::setParameter(const std::string& name, const std::string& value) {
	m_stringParameters[name] = value;
	this->log("String parameter " + name + " set to " + value);
}

std::string IPack::getStringParameter(const std::string& name) const {
	auto it = m_stringParameters.find(name);
	if (it != m_stringParameters.end()) {
		return it->second;
	}
	return "";
}

// ISettings implementation
json IPack::getSettings() const {
	json settings;
	settings["name"] = m_name;
	settings["description"] = m_description;
	settings["license"] = m_license;
	settings["url"] = m_url;
	settings["version"] = m_version;
	settings["enabled"] = m_enabled;
	settings["initialized"] = m_initialized;
	settings["time"] = m_time;
	settings["dependencies"] = m_dependencies;
	settings["parameters"] = m_parameters;
	settings["stringParameters"] = m_stringParameters;
	settings["customSettings"] = m_settings;
	return settings;
}

void IPack::setSettings(const json& settings) {
	if (settings.contains("name"))
		m_name = settings["name"];
	if (settings.contains("description"))
		m_description = settings["description"];
	if (settings.contains("license"))
		m_license = settings["license"];
	if (settings.contains("url"))
		m_url = settings["url"];
	if (settings.contains("version")) {
		if (settings["version"].is_string())
			m_version = settings["version"].get<std::string>();
		else if (settings["version"].is_number())
			m_version = std::to_string(settings["version"].get<double>());
	}
	if (settings.contains("enabled"))
		m_enabled = settings["enabled"];
	if (settings.contains("time"))
		m_time = settings["time"];
	if (settings.contains("dependencies"))
		m_dependencies = settings["dependencies"];
	if (settings.contains("parameters"))
		m_parameters = settings["parameters"];
	if (settings.contains("stringParameters"))
		m_stringParameters = settings["stringParameters"];
	if (settings.contains("customSettings"))
		m_settings = settings["customSettings"];
}

// -----------------------------------------------------------------------------
// Template-method public wrappers
// -----------------------------------------------------------------------------

void IPack::rigInit() {
	// Framework-level initialization
	this->log("Initializing pack");

	// Call user hook
	m_initialized = this->init();

	if (m_initialized) {
		this->log("Pack initialized successfully");
	} else {
		this->error("Pack initialization failed");
	}
}

void IPack::rigSetup() {
	// Framework-level setup
	this->log("Setting up pack");

	// Call user hook
	this->setup();
}

void IPack::rigUpdate(float deltaTime) {
	// Framework-level update
	m_time += deltaTime;

	// Call user hook
	this->update(deltaTime);
}

void IPack::rigDraw() {
	// Framework-level draw
	// Call user hook
	this->draw();
}

void IPack::rigCleanup() {
	// Framework-level cleanup
	this->log("Cleaning up pack");

	// Call user hook
	this->cleanup();

	m_initialized = false;
}

RigKitEngine* IPack::getEngine() const {
	return RigKitEngine::getEngine();
}

IPack* IPack::lookupPack(const std::string& name) const {
	auto* engine = getEngine();
	if (!engine) {
		return nullptr;
	}
	auto* packs = engine->getPackManager();
	if (!packs) {
		return nullptr;
	}
	auto pack = packs->getPack(name);
	return pack ? pack.get() : nullptr;
}

} // namespace rigkit
