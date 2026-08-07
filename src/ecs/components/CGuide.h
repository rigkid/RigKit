#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../PropertyReflection.h"

namespace rigkit {
namespace ecs {

struct CGuide {
	float position;
	bool vertical; // true = vertical, false = horizontal
	glm::vec4 color;
};

} // namespace ecs
} // namespace rigkit
