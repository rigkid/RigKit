#pragma once

#include <cstdint>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief One glyph in a face. Speaks `rig.font.glyph`.
 * @details Glyph name is `CName.stableId`. Contours compose `CPath` on this
 * entity (font units, y-up). Sidebearings follow path bounds and `width`.
 * Unicodes stay off the inspector (variable-length); the glyph list shows them.
 */
struct CFontGlyph {
	std::vector<uint32_t> unicodes;
	float width = 0.f;

	std::vector<sProp> GetProperties() { return {{0, "Width", EPT_FLOAT, &width}}; }
};

} // namespace ecs
} // namespace rigkit
