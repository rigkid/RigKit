#pragma once

#include <functional>
#include <string>
#include <vector>

#include "core/IManager.h"
#include "core/ISettings.h"
#include "core/json.h"
#include "ecs/PropertyReflection.h"

namespace rigkit {

/// Lightweight view of a registered preference section (for Preferences UI).
struct PrefSectionView {
	std::string id;
	std::string label;
};

/**
 * @brief User preference registry + persistence to AppPaths::getUserSettingsFile().
 * @details Packs register POD structs with GetProperties(); values load from
 *          `<userData>/user/rigkit_settings.json` on register and save on request / exit.
 */
class MSettings : public IManager, public ISettings {
  public:
	MSettings();
	void init() override {}
	void shutdown() override {}

	/// Load JSON blob from disk (does not require registered sections yet).
	bool loadFromDisk();
	/// Serialize registered sections (merge into blob) and write disk.
	bool saveToDisk();

	bool saveSettings(const json& settings);
	bool loadSettings(json& outSettings);

	template <typename T>
	void registerPreferences(const std::string& id, const std::string& label, T* data,
							 std::function<void()> onChanged = nullptr) {
		static_assert(has_get_properties<T>::value, "Preference POD must provide GetProperties()");
		registerPreferencesImpl(
			id, label, [data]() { return data->GetProperties(); }, std::move(onChanged));
	}

	void unregisterPreferences(const std::string& id);

	std::vector<PrefSectionView> preferenceSections() const;
	std::vector<sProp> preferenceProperties(const std::string& id);
	void notifyPreferenceChanged(const std::string& id);

	void markDirty() { m_dirty = true; }
	void clearDirty() { m_dirty = false; }
	bool isDirty() const { return m_dirty; }

	/**
	 * @brief Non-section values in the settings blob (e.g. recentFiles).
	 * @details Preserved across saveToDisk alongside preference sections.
	 *          Key "sections" is reserved and rejected.
	 */
	void setValue(const std::string& key, const json& value);
	json getValue(const std::string& key) const;

	json getSettings() const override;
	void setSettings(const json& settings) override;

  private:
	struct Section {
		std::string id;
		std::string label;
		std::function<std::vector<sProp>()> properties;
		std::function<void()> onChanged;
	};

	void registerPreferencesImpl(const std::string& id, const std::string& label,
								 std::function<std::vector<sProp>()> properties,
								 std::function<void()> onChanged);
	void applySectionFromBlob(const Section& section);
	Section* findSection(const std::string& id);
	const Section* findSection(const std::string& id) const;

	std::vector<Section> m_sections;
	json m_blob = json::object();
	bool m_dirty = false;
};

} // namespace rigkit
