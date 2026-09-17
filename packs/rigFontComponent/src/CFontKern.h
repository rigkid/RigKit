#pragma once

#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief One kerning pair. Speaks `rig.font.kern`.
 * @details `left` / `right` are glyph or group names, matching UFO plist keys.
 */
struct CFontKern {
	std::string left;
	std::string right;
	float value = 0.f;

	std::vector<sProp> GetProperties() {
		return {{0, "Left", EPT_STRING, &left},
				{1, "Right", EPT_STRING, &right},
				{2, "Value", EPT_FLOAT, &value}};
	}
};

} // namespace ecs
} // namespace rigkit
