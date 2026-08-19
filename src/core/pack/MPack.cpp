#include "MPack.h"

#include "core/util/AppPaths.h"
#include "IPack.h"
#include "ISettings.h"
#include "json.h"
#include "PackRegistry.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace {

using rigkit::json;

void applyPackManifestFields(rigkit::IPack& pack, const json& manifest) {
	if (manifest.contains("description") && manifest["description"].is_string()) {
		pack.setDescription(manifest["description"].get<std::string>());
	}
	if (manifest.contains("license") && manifest["license"].is_string()) {
		pack.setLicense(manifest["license"].get<std::string>());
	}
	if (manifest.contains("url") && manifest["url"].is_string()) {
		pack.setUrl(manifest["url"].get<std::string>());
	}
	if (manifest.contains("version")) {
		if (manifest["version"].is_string()) {
			pack.setVersion(manifest["version"].get<std::string>());
		} else if (manifest["version"].is_number()) {
			pack.setVersion(std::to_string(manifest["version"].get<double>()));
		}
	}
	// Runtime init order — same names as CMake/CPM (string or { "name": ... }).
	if (manifest.contains("dependencies") && manifest["dependencies"].is_array()) {
		std::vector<std::string> deps;
		deps.reserve(manifest["dependencies"].size());
		for (const auto& d : manifest["dependencies"]) {
			if (d.is_string()) {
				deps.push_back(d.get<std::string>());
			} else if (d.is_object() && d.contains("name") && d["name"].is_string()) {
				deps.push_back(d["name"].get<std::string>());
			}
		}
		pack.setDependencies(std::move(deps));
	}
}

/** @return First existing pack.json for @p name, or empty. */
std::string findPackManifestPath(const std::string& name) {
	namespace fs = std::filesystem;
	if (name.empty()) {
		return {};
	}

	// Shipped next to the exe: <exeDir>/data/packs/<name>/pack.json
	const fs::path deployed = fs::path(AppPaths::getDataDir()) / "packs" / name / "pack.json";
	std::error_code ec;
	if (fs::is_regular_file(deployed, ec)) {
		return deployed.lexically_normal().string();
	}

	// Dev / smoke: walk up from the exe looking for packs/<name>/pack.json
	fs::path dir = fs::path(AppPaths::getExecutableDir());
	for (int i = 0; i < 8 && !dir.empty(); ++i) {
		const fs::path candidate = dir / "packs" / name / "pack.json";
		if (fs::is_regular_file(candidate, ec)) {
			return candidate.lexically_normal().string();
		}
		const fs::path parent = dir.parent_path();
		if (parent == dir) {
			break;
		}
		dir = parent;
	}
	return {};
}

void applyPackManifestFile(rigkit::IPack& pack) {
	const std::string path = findPackManifestPath(pack.getName());
	if (path.empty()) {
		return;
	}
	try {
		std::ifstream f(path);
		if (!f.is_open()) {
			return;
		}
		json manifest;
		f >> manifest;
		applyPackManifestFields(pack, manifest);
	} catch (const std::exception& e) {
		spdlog::warn("Pack '{}': failed to read {}: {}", pack.getName(), path, e.what());
	}
}

} // namespace

rigkit::MPack::MPack(rigkit::RigKitEngine* engine) : m_engine(engine), m_packDirectory("packs") {}

rigkit::MPack::~MPack() {
	cleanupAll();
}

void rigkit::MPack::unregisterPack(const std::string& name) {
	auto it = m_packs.find(name);
	if (it != m_packs.end()) {
		it->second->cleanup();
		m_packs.erase(it);

		// Remove from order
		auto orderIt = std::find(m_packOrder.begin(), m_packOrder.end(), name);
		if (orderIt != m_packOrder.end()) {
			m_packOrder.erase(orderIt);
		}

		spdlog::info("Unregistered pack: {}", name);
	}
}

std::shared_ptr<rigkit::IPack> rigkit::MPack::getPack(const std::string& name) const {
	auto it = m_packs.find(name);
	if (it != m_packs.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<rigkit::IPack> rigkit::MPack::findPackByType(const std::type_info& type) const {
	// m_packOrder may carry dependency names that were never registered.
	for (const auto& name : m_packOrder) {
		auto it = m_packs.find(name);
		if (it != m_packs.end() && it->second && typeid(*it->second) == type) {
			return it->second;
		}
	}
	return nullptr;
}

bool rigkit::MPack::initAll() {
	spdlog::info("Initializing all packs...");

	// Sort packs by dependencies
	sortPacksByDependencies();

	// Check for circular dependencies
	auto circular = getCircularDependencies();
	if (!circular.empty()) {
		spdlog::error("Circular dependencies detected:");
		for (const auto& dep : circular) {
			spdlog::error("  {}", dep);
		}
		return false;
	}

	// Initialize packs in order
	for (const auto& name : m_packOrder) {
		auto pack = m_packs[name];
		if (pack && pack->isEnabled()) {
			spdlog::info("Initializing pack: {}", name);
			pack->rigInit();
			if (!pack->isInitialized()) {
				spdlog::error("Failed to initialize pack: {}", name);
				return false;
			}
		}
	}

	spdlog::info("All packs initialized successfully");
	return true;
}

void rigkit::MPack::setupAll() {
	for (const auto& name : m_packOrder) {
		auto pack = m_packs[name];
		if (pack && pack->isEnabled() && pack->isInitialized()) {
			pack->rigSetup();
		}
	}
}

void rigkit::MPack::updateAll(float deltaTime) {
	for (const auto& name : m_packOrder) {
		auto pack = m_packs[name];
		if (pack && pack->isEnabled() && pack->isInitialized()) {
			pack->rigUpdate(deltaTime);
		}
	}
}

void rigkit::MPack::drawAll() {
	for (const auto& name : m_packOrder) {
		auto pack = m_packs[name];
		if (pack && pack->isEnabled() && pack->isInitialized()) {
			pack->rigDraw();
		}
	}
}

void rigkit::MPack::cleanupAll() {
	spdlog::info("Cleaning up all packs...");
	for (auto it = m_packOrder.rbegin(); it != m_packOrder.rend(); ++it) {
		auto pack = m_packs[*it];
		if (pack && pack->isInitialized()) {
			pack->rigCleanup();
		}
	}
	spdlog::info("All packs cleaned up");
}

void rigkit::MPack::initDefaultPacks() {
	// This would initialize default packs that are always available
	spdlog::info("Initializing default packs...");
}

void rigkit::MPack::loadDefaultPacks() {
	// This would load packs that are always enabled by default
	spdlog::info("Loading default packs...");
}

void rigkit::MPack::scanPackDirectory(const std::string& directory) {
	spdlog::info("Scanning pack directory: {}", directory);

	if (!std::filesystem::exists(directory)) {
		spdlog::warn("Pack directory does not exist: {}", directory);
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(directory)) {
		if (entry.is_directory()) {
			std::string packPath = entry.path().string();
			std::string packName = entry.path().filename().string();

			// Check for pack.json manifest
			std::ostringstream oss;
			oss << packPath << "/pack.json";
			std::string manifestPath = oss.str();
			if (std::filesystem::exists(manifestPath)) {
				spdlog::info("Found pack manifest: {}", manifestPath);
				loadFromManifest(manifestPath);
			}
		}
	}
}

bool rigkit::MPack::loadPack(const std::string& path) {
	spdlog::info("Loading pack from path: {}", path);

	// Try to load from manifest first
	std::ostringstream oss1;
	oss1 << path << "/pack.json";
	std::string manifestPath = oss1.str();
	if (std::filesystem::exists(manifestPath)) {
		return loadFromManifest(manifestPath);
	}

	// Single-file / DLL packs are not a host path — packs load via pack.json + static link.
	if (std::filesystem::exists(path)) {
		spdlog::warn("Direct pack file load unsupported (use pack.json / STATIC packs): {}", path);
		return false;
	}

	return false;
}

void rigkit::MPack::reloadPack(const std::string& name) {
	spdlog::info("Reloading pack: {}", name);

	auto pack = getPack(name);
	if (pack) {
		pack->rigCleanup();
		pack->rigInit();
		if (pack->isInitialized()) {
			pack->rigSetup();
		}
	}
}

void rigkit::MPack::reloadAllPacks() {
	spdlog::info("Reloading all packs...");
	cleanupAll();
	initAll();
	setupAll();
}

bool rigkit::MPack::loadFromManifest(const std::string& path) {
	spdlog::info("Loading pack from manifest: {}", path);

	try {
		std::ifstream f(path);
		if (!f.is_open()) {
			spdlog::error("Could not open manifest file: {}", path);
			return false;
		}

		json manifest;
		f >> manifest;

		if (!manifest.contains("name")) {
			spdlog::error("Manifest missing 'name' field: {}", path);
			return false;
		}

		std::string packName = manifest["name"];
		spdlog::info("Loading pack: {}", packName);

		// Try to create the pack using the registry
		auto pack = PackRegistry::instance().create(packName);
		if (pack) {
			applyPackManifestFields(*pack, manifest);
			registerPack(pack);
			return true;
		} else {
			spdlog::error("Could not create pack: {}", packName);
			return false;
		}
	} catch (const std::exception& e) {
		spdlog::error("Error loading manifest {}: {}", path, e.what());
		return false;
	}
}

void rigkit::MPack::enablePack(const std::string& name) {
	auto pack = getPack(name);
	if (pack) {
		pack->setEnabled(true);
		m_packConfig[name] = true;
		spdlog::info("Enabled pack: {}", name);
	}
}

void rigkit::MPack::disablePack(const std::string& name) {
	auto pack = getPack(name);
	if (pack) {
		pack->setEnabled(false);
		m_packConfig[name] = false;
		spdlog::info("Disabled pack: {}", name);
	}
}

bool rigkit::MPack::isPackEnabled(const std::string& name) const {
	auto pack = getPack(name);
	return pack ? pack->isEnabled() : false;
}

std::vector<std::string> rigkit::MPack::getPackNames() const {
	std::vector<std::string> names;
	for (const auto& pair : m_packs) {
		names.push_back(pair.first);
	}
	return names;
}

std::vector<std::shared_ptr<rigkit::IPack>> rigkit::MPack::getEnabledPacks() const {
	std::vector<std::shared_ptr<IPack>> enabled;
	for (const auto& pair : m_packs) {
		if (pair.second && pair.second->isEnabled()) {
			enabled.push_back(pair.second);
		}
	}
	return enabled;
}

std::vector<std::shared_ptr<rigkit::IPack>> rigkit::MPack::getAllPacks() const {
	std::vector<std::shared_ptr<IPack>> all;
	for (const auto& pair : m_packs) {
		if (pair.second) {
			all.push_back(pair.second);
		}
	}
	return all;
}

bool rigkit::MPack::resolveDependencies() {
	spdlog::info("Resolving pack dependencies...");

	// Check all pack dependencies
	for (const auto& pair : m_packs) {
		if (!checkDependencies(pair.first)) {
			spdlog::error("Dependency resolution failed for pack: {}", pair.first);
			return false;
		}
	}

	spdlog::info("All dependencies resolved successfully");
	return true;
}

std::vector<std::string> rigkit::MPack::getPackDependencies(const std::string& name) const {
	auto pack = getPack(name);
	if (pack) {
		return pack->getDependencies();
	}
	return {};
}

void rigkit::MPack::savePackConfig() {
	spdlog::info("Saving pack configuration...");

	json config;
	for (const auto& pair : m_packs) {
		config[pair.first] = {{"enabled", pair.second->isEnabled()},
							  {"settings", pair.second->getSettings()}};
	}

	// Save to file
	std::ofstream f("pack_config.json");
	if (f.is_open()) {
		f << config.dump(2);
		spdlog::info("Pack configuration saved");
	} else {
		spdlog::error("Could not save pack configuration");
	}
}

void rigkit::MPack::loadPackConfig() {
	spdlog::info("Loading pack configuration...");

	std::ifstream f("pack_config.json");
	if (!f.is_open()) {
		spdlog::warn("No pack configuration file found");
		return;
	}

	try {
		json config;
		f >> config;

		for (const auto& [name, packConfig] : config.items()) {
			auto pack = getPack(name);
			if (pack) {
				if (packConfig.contains("enabled")) {
					pack->setEnabled(packConfig["enabled"]);
				}
				if (packConfig.contains("settings")) {
					pack->setSettings(packConfig["settings"]);
				}
			}
		}

		spdlog::info("Pack configuration loaded");
	} catch (const std::exception& e) {
		spdlog::error("Error loading pack configuration: {}", e.what());
	}
}

void rigkit::MPack::setPackDirectory(const std::string& directory) {
	m_packDirectory = directory;
}

void rigkit::MPack::addGlobalEventListener(const std::string& event,
										   std::function<void()> callback) {
	m_globalEventListeners[event].push_back(callback);
}

void rigkit::MPack::triggerGlobalEvent(const std::string& event) {
	auto it = m_globalEventListeners.find(event);
	if (it != m_globalEventListeners.end()) {
		for (auto& callback : it->second) {
			callback();
		}
	}
}

rigkit::json rigkit::MPack::getSettings() const {
	json settings;
	settings["packDirectory"] = m_packDirectory;
	settings["packConfig"] = m_packConfig;
	settings["packOrder"] = m_packOrder;

	json packs;
	for (const auto& pair : m_packs) {
		packs[pair.first] = pair.second->getSettings();
	}
	settings["packs"] = packs;

	return settings;
}

void rigkit::MPack::setSettings(const json& settings) {
	if (settings.contains("packDirectory")) {
		m_packDirectory = settings["packDirectory"];
	}
	if (settings.contains("packConfig")) {
		m_packConfig = settings["packConfig"];
	}
	if (settings.contains("packOrder")) {
		m_packOrder = settings["packOrder"];
	}
	if (settings.contains("packs")) {
		for (const auto& [name, packSettings] : settings["packs"].items()) {
			auto pack = getPack(name);
			if (pack) {
				pack->setSettings(packSettings);
			}
		}
	}
}

bool rigkit::MPack::checkDependencies(const std::string& packName) {
	auto pack = getPack(packName);
	if (!pack) {
		return false;
	}

	for (const auto& dep : pack->getDependencies()) {
		auto depPack = getPack(dep);
		if (!depPack) {
			spdlog::error("Missing dependency '{}' for pack '{}'", dep, packName);
			return false;
		}
		if (!depPack->isEnabled()) {
			spdlog::error("Dependency '{}' is disabled for pack '{}'", dep, packName);
			return false;
		}
	}

	return true;
}

void rigkit::MPack::sortPacksByDependencies() {
	// Simple topological sort implementation
	std::vector<std::string> sorted;
	std::unordered_set<std::string> visited;
	std::unordered_set<std::string> temp;

	std::function<void(const std::string&)> visit = [&](const std::string& name) {
		if (temp.find(name) != temp.end()) {
			// Circular dependency detected
			return;
		}
		if (visited.find(name) != visited.end()) {
			return;
		}

		temp.insert(name);

		auto pack = getPack(name);
		if (pack) {
			for (const auto& dep : pack->getDependencies()) {
				visit(dep);
			}
		}

		temp.erase(name);
		visited.insert(name);
		sorted.push_back(name);
	};

	for (const auto& pair : m_packs) {
		visit(pair.first);
	}

	m_packOrder = sorted;
}

std::vector<std::string> rigkit::MPack::getCircularDependencies() const {
	// This is a simplified implementation
	// A full implementation would use a proper cycle detection algorithm
	std::vector<std::string> circular;

	for (const auto& pair : m_packs) {
		std::shared_ptr<IPack> pack = pair.second;
		if (pack) {
			const std::vector<std::string>& deps = pack->getDependencies();
			for (const auto& dep : deps) {
				auto depPack = getPack(dep);
				if (depPack) {
					const std::vector<std::string>& depDeps = depPack->getDependencies();
					for (const auto& depDep : depDeps) {
						if (depDep == pair.first) {
							std::ostringstream oss;
							oss << pair.first << " -> " << dep << " -> " << depDep;
							circular.push_back(oss.str());
						}
					}
				}
			}
		}
	}

	return circular;
}

void rigkit::MPack::registerPack(std::shared_ptr<rigkit::IPack> pack) {
	if (!pack) {
		spdlog::error("Attempted to register null pack");
		return;
	}

	// pack.json owns About identity + runtime dependency names.
	applyPackManifestFile(*pack);

	std::string name = pack->getName();
	if (m_packs.find(name) != m_packs.end()) {
		spdlog::warn("Pack '{}' already registered, replacing", name);
	}

	m_packs[name] = pack;
	m_packOrder.push_back(name);

	// Set default enabled state
	if (m_packConfig.find(name) == m_packConfig.end()) {
		m_packConfig[name] = true;
	}

	spdlog::info("Registered pack: {}", name);
}
