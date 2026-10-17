#pragma once

#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Axis-aligned box solid. Speaks `rig.cad.box`.
 * @details CSG source of truth; `CMesh` on the same entity is an optional bake.
 */
struct CCadBox {
	float sizeX = 1.f;
	float sizeY = 1.f;
	float sizeZ = 1.f;
	bool center = true;

	std::vector<sProp> GetProperties() {
		return {{0, "Size X", EPT_FLOAT, &sizeX},
				{1, "Size Y", EPT_FLOAT, &sizeY},
				{2, "Size Z", EPT_FLOAT, &sizeZ},
				{3, "Center", EPT_BOOL, &center}};
	}
};

} // namespace ecs
} // namespace rigkit
