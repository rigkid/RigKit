#pragma once

#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Circular cylinder solid (height along +Y). Speaks `rig.cad.cylinder`.
 * @details `circularSegments` 0 = host default. `CMesh` on the same entity is an optional bake.
 */
struct CCadCylinder {
	float radius = 1.f;
	float height = 1.f;
	int circularSegments = 0;
	bool center = true;

	std::vector<sProp> GetProperties() {
		return {{0, "Radius", EPT_FLOAT, &radius},
				{1, "Height", EPT_FLOAT, &height},
				{2, "Circular segments", EPT_INT, &circularSegments},
				{3, "Center", EPT_BOOL, &center}};
	}
};

} // namespace ecs
} // namespace rigkit
