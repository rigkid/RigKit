#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "ISettings.h"

namespace rigkit {

// Forward declaration for engine access
class RigKitEngine;

class IPack : public ISettings {
  public:
	IPack(const std::string& name);
	virtual ~IPack() = default;

	// ----------------------------------------
	// Framework entry points DO NOT override.
	// ----------------------------------------
	void rigInit();
	void rigSetup();
	void rigUpdate(float deltaTime);
	void rigDraw();
	void rigCleanup();

	// Pack information
	const std::string& getName() const { return m_name; }
	const std::string& getDescription() const { return m_description; }
	const std::string& getLicense() const { return m_license; }

	// Pack state
	bool isEnabled() const { return m_enabled; }
	void setEnabled(bool enabled) { m_enabled = enabled; }
	bool isInitialized() const { return m_initialized; }

	// Dependencies
	void addDependency(const std::string& packName);
	const std::vector<std::string>& getDependencies() const { return m_dependencies; }

	// Configuration
	void setDescription(const std::string& description) { m_description = description; }
	void setLicense(const std::string& license) { m_license = license; }

	// Engine access
	RigKitEngine* getEngine() const;

	// Utility functions for packs
	template <typename T> T* getPack(const std::string& name);

	// Event system
	void addEventListener(const std::string& event, std::function<void()> callback);
	void removeEventListener(const std::string& event);
	void triggerEvent(const std::string& event);

	// Parameter system (common to many packs)
	void setParameter(const std::string& name, float value);
	float getParameter(const std::string& name) const;
	void onParameterChanged(const std::string& name, std::function<void(float)> callback);
	void setParameter(const std::string& name, const std::string& value);
	std::string getStringParameter(const std::string& name) const;

	// Time tracking (common to many packs)
	float getTime() const { return m_time; }
	void resetTime() { m_time = 0.0f; }

	// ISettings interface
	json getSettings() const override;
	void setSettings(const json& settings) override;

  public:
	// ----------------------------------------
	// User hooks – override these in your pack
	// ----------------------------------------
	virtual bool init() { return true; }
	virtual void setup() {}
	virtual void update(float deltaTime) { (void)deltaTime; }
	virtual void draw() {}
	virtual void cleanup() {}

  protected:
	// Pack state
	std::string m_name;
	std::string m_description;
	std::string m_license;
	bool m_enabled;
	bool m_initialized;

	// Dependencies
	std::vector<std::string> m_dependencies;

	// Event system
	std::unordered_map<std::string, std::vector<std::function<void()>>> m_eventListeners;

	// Common pack functionality
	float m_time;
	std::unordered_map<std::string, float> m_parameters;
	std::unordered_map<std::string, std::string> m_stringParameters;
	std::unordered_map<std::string, std::function<void(float)>> m_parameterCallbacks;

	// Settings storage
	json m_settings;

	// Helper functions for packs
	void log(const std::string& message);
	void error(const std::string& message);
	void warning(const std::string& message);

  private:
	IPack* lookupPack(const std::string& name) const;
};

template <typename T> T* IPack::getPack(const std::string& name) {
	return dynamic_cast<T*>(lookupPack(name));
}

} // namespace rigkit