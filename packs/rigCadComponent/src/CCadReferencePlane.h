#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Unbounded flat datum to build against. Speaks `rig.cad.reference_plane`.
 * @details Not a solid, never a CSG operand, and it does not cut anything. The
 * plane has no edges - a host draws whatever patch suits the view. Naming a part
 * in `a` moves the plane onto it so the datum follows it when the model
 * rebuilds, and `origin` is ignored; `normal` stays authored either way, since
 * one part fixes a point but not an orientation. A missing named part, or a
 * zero-length normal, means the plane is skipped.
 */
struct CCadReferencePlane {
	glm::vec3 origin{0.f, 0.f, 0.f};
	glm::vec3 normal{0.f, 1.f, 0.f};
	std::string a;

	std::vector<sProp> GetProperties() {
		return {{0, "Origin", EPT_VEC3, &origin},
				{1, "Normal", EPT_VEC3, &normal},
				{2, "A", EPT_STRING, &a}};
	}
};

} // namespace ecs
} // namespace rigkit
