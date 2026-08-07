#pragma once

#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>
#include "IManager.h"
#include "ISettings.h"

namespace rigkit {
class RigKitEngine;
class IPack;
class MPack : public IManager, public ISettings {
  public:
	explicit MPack(RigKitEngine* engine);
	~MPack();
	void init() override {}
	void shutdown() override {}

	// Pack registration and management
	void registerPack(std::shared_ptr<IPack> pack);

	/** @brief Construct and register a pack; MPack owns the instance. */
	template <typename T, typename... Args> void registerPack(Args&&... args) {
		registerPack(std::make_shared<T>(std::forward<Args>(args)...));
	}

	void unregisterPack(const std::string& name);
	std::shared_ptr<IPack> getPack(const std::string& name) const;

	/**
	 * @brief Typed lookup by pack class after registerPack — MPack owns the instance.
	 * @return The registered pack, or nullptr when that class was never registered.
	 */
	template <typename T> std::shared_ptr<T> getPack() const {
		return std::dynamic_pointer_cast<T>(findPackByType(typeid(T)));
	}

	// Pack load / unload
	bool initAll();
	void setupAll();
	void updateAll(float deltaTime);
	void drawAll();
	void cleanupAll();

	// Default pack initialization
	void initDefaultPacks();
	void loadDefaultPacks();

	// Pack discovery and loading
	void scanPackDirectory(const std::string& directory);
	bool loadPack(const std::string& path);
	void reloadPack(const std::string& name);
	void reloadAllPacks();
	bool loadFromManifest(const std::string& path);

	// Pack state management
	void enablePack(const std::string& name);
	void disablePack(const std::string& name);
	bool isPackEnabled(const std::string& name) const;

	// Pack information
	std::vector<std::string> getPackNames() const;
	std::vector<std::shared_ptr<IPack>> getEnabledPacks() const;
	std::vector<std::shared_ptr<IPack>> getAllPacks() const;

	// Dependency resolution
	bool resolveDependencies();
	std::vector<std::string> getPackDependencies(const std::string& name) const;

	// Pack configuration
	void savePackConfig();
	void loadPackConfig();

	// Utility functions
	void setPackDirectory(const std::string& directory);
	const std::string& getPackDirectory() const { return m_packDirectory; }

	// Event system
	void addGlobalEventListener(const std::string& event, std::function<void()> callback);
	void triggerGlobalEvent(const std::string& event);

	// ISettings interface
	json getSettings() const override;
	void setSettings(const json& settings) override;

  private:
	rigkit::RigKitEngine* m_engine = nullptr;

	// Pack storage
	std::unordered_map<std::string, std::shared_ptr<IPack>> m_packs;
	std::vector<std::string> m_packOrder; // Order for initialization

	// Configuration
	std::string m_packDirectory;
	std::unordered_map<std::string, bool> m_packConfig; // enabled/disabled state

	// Event system
	std::unordered_map<std::string, std::vector<std::function<void()>>> m_globalEventListeners;

	// Helper functions
	std::shared_ptr<IPack> findPackByType(const std::type_info& type) const;
	bool checkDependencies(const std::string& packName);
	void sortPacksByDependencies();
	std::vector<std::string> getCircularDependencies() const;
};
} // namespace rigkit