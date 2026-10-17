#pragma once

#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Solid from a 2D profile spun about +Y. Speaks `rig.cad.revolve`.
 * @details `profile` is an entity name. `circularSegments` 0 = host default.
 * Absent `revolveDegrees` on the wire means 360.
 */
struct CCadRevolve {
	std::string profile;
	float revolveDegrees = 360.f;
	int circularSegments = 0;

	std::vector<sProp> GetProperties() {
		return {{0, "Profile", EPT_STRING, &profile},
				{1, "Revolve degrees", EPT_FLOAT, &revolveDegrees},
				{2, "Circular segments", EPT_INT, &circularSegments}};
	}
};

} // namespace ecs
} // namespace rigkit
