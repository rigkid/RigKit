#pragma once

#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief One design axis on a face. Speaks `rig.font.axis`.
 * @details Child of the face. Live instance values stay host session state.
 */
struct CFontAxis {
	std::string tag;
	std::string name;
	float min = 0.f;
	float defaultValue = 0.f;
	float max = 1.f;

	std::vector<sProp> GetProperties() {
		return {{0, "Tag", EPT_STRING, &tag},
				{1, "Name", EPT_STRING, &name},
				{2, "Min", EPT_FLOAT, &min},
				{3, "Default", EPT_FLOAT, &defaultValue},
				{4, "Max", EPT_FLOAT, &max}};
	}
};

} // namespace ecs
} // namespace rigkit
