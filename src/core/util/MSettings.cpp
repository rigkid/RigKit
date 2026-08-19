#include "core/util/MSettings.h"

#include "core/util/AppPaths.h"
#include "ecs/PropertyJson.h"

#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

namespace rigkit {

MSettings::MSettings() = default;

bool MSettings::saveSettings(const rigkit::json& settings) {
	const std::string settingsFile = AppPaths::getUserSettingsFile();
	std::error_code ec;
	std::filesystem::create_directories(std::filesystem::path(settingsFile).parent_path(), ec);
	std::ofstream out(settingsFile);
	if (out.is_open()) {
		out << settings.dump(2);
		spdlog::info("[Settings] Saved settings to {}", settingsFile);
		return true;
	}
	spdlog::error("[Settings] Failed to save settings to {}", settingsFile);
	return false;
}

bool MSettings::loadSettings(rigkit::json& outSettings) {
	const std::string settingsFile = AppPaths::getUserSettingsFile();
	std::ifstream in(settingsFile);
	if (!in.is_open()) {
		spdlog::info("[Settings] No settings file found, using defaults");
		outSettings = rigkit::json::object();
		return false;
	}
	try {
		in >> outSettings;
		spdlog::info("[Settings] Loaded settings from {}", settingsFile);
		return true;
	} catch (const std::exception& e) {
		spdlog::error("[Settings] Error loading settings: {}", e.what());
		outSettings = rigkit::json::object();
		return false;
	}
}

bool MSettings::loadFromDisk() {
	const bool ok = loadSettings(m_blob);
	if (!m_blob.is_object()) {
		m_blob = json::object();
	}
	if (!m_blob.contains("sections") || !m_blob["sections"].is_object()) {
		m_blob["sections"] = json::object();
	}
	for (const auto& section : m_sections) {
		applySectionFromBlob(section);
	}
	m_dirty = false;
	return ok;
}

bool MSettings::saveToDisk() {
	json registered = getSettings();
	if (!m_blob.is_object()) {
		m_blob = json::object();
	}
	if (!m_blob.contains("sections") || !m_blob["sections"].is_object()) {
		m_blob["sections"] = json::object();
	}
	if (registered.contains("sections") && registered["sections"].is_object()) {
		for (auto it = registered["sections"].begin(); it != registered["sections"].end(); ++it) {
			m_blob["sections"][it.key()] = it.value();
		}
	}
	const bool ok = saveSettings(m_blob);
	if (ok) {
		m_dirty = false;
	}
	return ok;
}

void MSettings::registerPreferencesImpl(const std::string& id, const std::string& label,
										std::function<std::vector<sProp>()> properties,
										std::function<void()> onChanged) {
	if (id.empty() || !properties) {
		spdlog::error("[Settings] registerPreferences requires non-empty id and properties");
		return;
	}
	if (Section* existing = findSection(id)) {
		existing->label = label;
		existing->properties = std::move(properties);
		existing->onChanged = std::move(onChanged);
		applySectionFromBlob(*existing);
		spdlog::info("[Settings] Updated preference section '{}'", id);
		return;
	}
	Section section;
	section.id = id;
	section.label = label;
	section.properties = std::move(properties);
	section.onChanged = std::move(onChanged);
	m_sections.push_back(std::move(section));
	applySectionFromBlob(m_sections.back());
	spdlog::info("[Settings] Registered preference section '{}'", id);
}

void MSettings::unregisterPreferences(const std::string& id) {
	for (auto it = m_sections.begin(); it != m_sections.end(); ++it) {
		if (it->id == id) {
			m_sections.erase(it);
			spdlog::info("[Settings] Unregistered preference section '{}'", id);
			return;
		}
	}
}

std::vector<PrefSectionView> MSettings::preferenceSections() const {
	std::vector<PrefSectionView> out;
	out.reserve(m_sections.size());
	for (const auto& section : m_sections) {
		out.push_back({section.id, section.label});
	}
	return out;
}

std::vector<sProp> MSettings::preferenceProperties(const std::string& id) {
	if (Section* section = findSection(id)) {
		return section->properties();
	}
	return {};
}

void MSettings::notifyPreferenceChanged(const std::string& id) {
	m_dirty = true;
	if (Section* section = findSection(id)) {
		if (section->onChanged) {
			section->onChanged();
		}
	}
}

void MSettings::setValue(const std::string& key, const json& value) {
	if (key.empty() || key == "sections") {
		spdlog::error("[Settings] setValue rejects empty key or reserved 'sections'");
		return;
	}
	if (!m_blob.is_object()) {
		m_blob = json::object();
	}
	m_blob[key] = value;
	m_dirty = true;
}

json MSettings::getValue(const std::string& key) const {
	if (key.empty() || !m_blob.is_object() || !m_blob.contains(key)) {
		return json();
	}
	return m_blob[key];
}

json MSettings::getSettings() const {
	json j;
	j["sections"] = json::object();
	for (const auto& section : m_sections) {
		auto props = section.properties();
		j["sections"][section.id] = propsToJson(props);
	}
	return j;
}

void MSettings::setSettings(const json& settings) {
	if (!settings.is_object()) {
		return;
	}
	m_blob = settings;
	if (!m_blob.contains("sections") || !m_blob["sections"].is_object()) {
		m_blob["sections"] = json::object();
	}
	for (const auto& section : m_sections) {
		applySectionFromBlob(section);
	}
	m_dirty = true;
}

void MSettings::applySectionFromBlob(const Section& section) {
	if (!m_blob.contains("sections") || !m_blob["sections"].is_object()) {
		return;
	}
	const auto& sections = m_blob["sections"];
	if (!sections.contains(section.id)) {
		return;
	}
	auto props = section.properties();
	jsonToProps(sections[section.id], props);
	if (section.onChanged) {
		section.onChanged();
	}
}

MSettings::Section* MSettings::findSection(const std::string& id) {
	for (auto& section : m_sections) {
		if (section.id == id) {
			return &section;
		}
	}
	return nullptr;
}

const MSettings::Section* MSettings::findSection(const std::string& id) const {
	for (const auto& section : m_sections) {
		if (section.id == id) {
			return &section;
		}
	}
	return nullptr;
}

} // namespace rigkit
