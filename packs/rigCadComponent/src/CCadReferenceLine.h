#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Unbounded straight datum to build against. Speaks `rig.cad.reference_line`.
 * @details Not a solid and never a CSG operand. The line has no ends  -
 * `origin` / `direction` say where it lies, and a host draws whatever span suits
 * the view. Naming two parts in `a` / `b` places the line through them instead,
 * so it follows the parts when the model rebuilds; `origin` / `direction` are
 * ignored then. A named part that no longer exists, or a zero-length direction,
 * means the line is skipped rather than drawn somewhere arbitrary.
 */
struct CCadReferenceLine {
	glm::vec3 origin{0.f, 0.f, 0.f};
	glm::vec3 direction{1.f, 0.f, 0.f};
	std::string a;
	std::string b;

	std::vector<sProp> GetProperties() {
		return {{0, "Origin", EPT_VEC3, &origin},
				{1, "Direction", EPT_VEC3, &direction},
				{2, "A", EPT_STRING, &a},
				{3, "B", EPT_STRING, &b}};
	}
};

} // namespace ecs
} // namespace rigkit
