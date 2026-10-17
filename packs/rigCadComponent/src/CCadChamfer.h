#pragma once

#include <vector>

#include "ecs/PropertyReflection.h"
#include "MeshEdge.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Bevel selected mesh edges. Speaks `rig.cad.chamfer`.
 * @details Compose on the solid entity. `allEdges` ignores `edges`. Authored intent  -
 * a mesh CSG kernel may approximate.
 */
struct CCadChamfer {
	float distance = 1.f;
	bool allEdges = false;
	std::vector<MeshEdge> edges;

	int propEdgeCount = 0;

	std::vector<sProp> GetProperties() {
		propEdgeCount = static_cast<int>(edges.size());
		return {{0, "Distance", EPT_FLOAT, &distance},
				{1, "All edges", EPT_BOOL, &allEdges},
				{2, "Named edges", EPT_INT, &propEdgeCount}};
	}
};

} // namespace ecs
} // namespace rigkit
