#pragma once

#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Solid from a 2D profile swept along +Z. Speaks `rig.cad.extrude`.
 * @details `profile` is an entity name (polygon / path / spline). `nDivisions` 0 = host default.
 */
struct CCadExtrude {
	std::string profile;
	float height = 1.f;
	int nDivisions = 0;
	float twistDegrees = 0.f;
	float scaleTop = 1.f;

	std::vector<sProp> GetProperties() {
		return {{0, "Profile", EPT_STRING, &profile},
				{1, "Height", EPT_FLOAT, &height},
				{2, "Divisions", EPT_INT, &nDivisions},
				{3, "Twist degrees", EPT_FLOAT, &twistDegrees},
				{4, "Scale top", EPT_FLOAT, &scaleTop}};
	}
};

} // namespace ecs
} // namespace rigkit
