#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Named point on a glyph (UFO anchor). Speaks `rig.font.anchor`.
 * @details Name lives on `CName`. Child of the glyph. Not `CAnchor` (3x3 cell).
 */
struct CFontAnchor {
	glm::vec2 point{0.f, 0.f};

	std::vector<sProp> GetProperties() { return {{0, "Point", EPT_VEC2, &point.x}}; }
};

} // namespace ecs
} // namespace rigkit
