#pragma once

#include <entt/entt.hpp>
#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Preset deck - playlist, output canvas, and switch policy.
 * @details Speaks `rig.visualizer.deck`. Assets are `CAssetRef` (`kind` preset).
 * The present pack draws into `output`. Live color / temperature are host
 * cache from picture analysis, not fields here. Milk mesh / hardcut stay
 * in the milk leaf pack.
 */
struct CVisualizer {
	enum class SwitchOn : int { Time = 0, Onset = 1, Beat = 2, Note = 3 };

	std::vector<entt::entity> presets;
	std::string presetDir = "data/presets";
	std::string presetPath;
	int index = 0;
	bool playing = true;
	bool shuffle = true;
	bool lockPreset = false;
	float presetDuration = 30.f;
	float transitionDuration = 2.7f;
	SwitchOn switchOn = SwitchOn::Time;
	entt::entity analysis{entt::null};
	entt::entity output{entt::null};
	int propPresetCount = 0;

	std::vector<sProp> GetProperties() {
		static const char* const kSwitch[] = {"time", "onset", "beat", "note"};
		propPresetCount = static_cast<int>(presets.size());
		return {{0, "playing", EPT_BOOL, &playing},
				{1, "shuffle", EPT_BOOL, &shuffle},
				{2, "lockPreset", EPT_BOOL, &lockPreset},
				{3, "index", EPT_INT, &index},
				{4, "presetDir", EPT_STRING, &presetDir},
				{5, "presetPath", EPT_STRING, &presetPath},
				{6, "presetDuration", EPT_FLOAT, &presetDuration},
				{7, "transitionDuration", EPT_FLOAT, &transitionDuration},
				{8, "switchOn", EPT_ENUM, reinterpret_cast<int*>(&switchOn), kSwitch, 4},
				{9, "presets", EPT_INT, &propPresetCount}};
	}
};

} // namespace ecs
} // namespace rigkit
