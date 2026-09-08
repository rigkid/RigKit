#pragma once

#include <memory>
#include <string>
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
	const std::string& getUrl() const { return m_url; }
	const std::string& getVersion() const { return m_version; }

	// Pack state
	bool isEnabled() const { return m_enabled; }
	void setEnabled(bool enabled) { m_enabled = enabled; }
	bool isInitialized() const { return m_initialized; }

	// Dependencies (prefer pack.json; applied by MPack::registerPack)
	void addDependency(const std::string& packName);
	void setDependencies(std::vector<std::string> deps);
	const std::vector<std::string>& getDependencies() const { return m_dependencies; }

	// Configuration (prefer pack.json; applied by MPack::registerPack)
	void setDescription(const std::string& description) { m_description = description; }
	void setLicense(const std::string& license) { m_license = license; }
	void setUrl(const std::string& url) { m_url = url; }
	void setVersion(const std::string& version) { m_version = version; }

	// Engine access
	RigKitEngine* getEngine() const;

	// Utility functions for packs
	template <typename T> T* getPack(const std::string& name);

	float getTime() const { return m_time; }
	void resetTime() { m_time = 0.0f; }

	// ISettings interface
	json getSettings() const override;
	void setSettings(const json& settings) override;

  public:
	// ----------------------------------------
	// User hooks - override these in your pack
	// ----------------------------------------
	virtual bool init() { return true; }
	virtual void setup() {}
	virtual void update(float deltaTime) { (void)deltaTime; }
	virtual void draw() {}
	virtual void cleanup() {}

	/**
	 * @brief Called after a host `IMui` attach or chrome swap.
	 * @details Re-register File/Edit/Export actions on the live UI manager.
	 */
	virtual void onUiAttached() {}

  protected:
	// Pack state
	std::string m_name;
	std::string m_description;
	std::string m_license;
	std::string m_url;
	std::string m_version;
	bool m_enabled;
	bool m_initialized;

	// Dependencies
	std::vector<std::string> m_dependencies;

	float m_time;

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
