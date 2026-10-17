#pragma once

#include <entt/entt.hpp>
#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Authored audio analysis request. Speaks `rig.audio.analysis`.
 * @details Live hop values (note, color, temperature, ...) stay on the
 * audio bus. `features` names which signals to publish.
 */
struct CAudioAnalysis {
	enum class Source : int { Input = 0, Asset = 1 };

	Source source = Source::Input;
	entt::entity asset{entt::null};
	int bandCount = 32;
	float smoothing = 0.f;
	float onsetThreshold = 0.f;
	std::vector<std::string> features;
	int propFeatureCount = 0;

	std::vector<sProp> GetProperties() {
		static const char* const kSource[] = {"input", "asset"};
		propFeatureCount = static_cast<int>(features.size());
		return {{0, "source", EPT_ENUM, reinterpret_cast<int*>(&source), kSource, 2},
				{1, "bandCount", EPT_INT, &bandCount},
				{2, "smoothing", EPT_FLOAT, &smoothing},
				{3, "onsetThreshold", EPT_FLOAT, &onsetThreshold},
				{4, "features", EPT_INT, &propFeatureCount}};
	}
};

} // namespace ecs
} // namespace rigkit
