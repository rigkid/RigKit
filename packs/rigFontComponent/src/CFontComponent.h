#pragma once

#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Composite reference - draw another glyph into this one.
 * @details Speaks `rig.font.component`. Lives on a child of the composite
 * glyph. `glyph` is a name (`CName.stableId` / entity name), not a handle.
 * The six numbers are the UFO 2x3 affine; omit identity at serialize.
 */
struct CFontComponent {
	std::string glyph;
	float scaleX = 1.f;
	float xyScale = 0.f;
	float yxScale = 0.f;
	float scaleY = 1.f;
	float offsetX = 0.f;
	float offsetY = 0.f;

	std::vector<sProp> GetProperties() {
		return {{0, "Glyph", EPT_STRING, &glyph},
				{1, "Scale X", EPT_FLOAT, &scaleX},
				{2, "Scale Y", EPT_FLOAT, &scaleY},
				{3, "Offset X", EPT_FLOAT, &offsetX},
				{4, "Offset Y", EPT_FLOAT, &offsetY}};
	}
};

} // namespace ecs
} // namespace rigkit
