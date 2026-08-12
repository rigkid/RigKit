#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "../PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Ruler / guide line on the canvas (position along an axis + colour).
 */
struct CGuide {
	float position = 0.f;
	bool vertical = true; // true = vertical, false = horizontal
	glm::vec4 color{0.3f, 0.6f, 1.f, 1.f};

	std::vector<sProp> GetProperties() {
		return {{0, "Position", EPT_FLOAT, &position},
				{1, "Vertical", EPT_BOOL, &vertical},
				{2, "Color", EPT_COLOR, &color}};
	}
};

} // namespace ecs
} // namespace rigkit
