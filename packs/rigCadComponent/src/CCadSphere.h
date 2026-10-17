#pragma once

#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Solid sphere at the local origin. Speaks `rig.cad.sphere`.
 * @details Distinct from `CSphere` (icosphere tessellation). `circularSegments` 0 = host default.
 */
struct CCadSphere {
	float radius = 1.f;
	int circularSegments = 0;

	std::vector<sProp> GetProperties() {
		return {{0, "Radius", EPT_FLOAT, &radius},
				{1, "Circular segments", EPT_INT, &circularSegments}};
	}
};

} // namespace ecs
} // namespace rigkit
