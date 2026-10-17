#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Numeric datum between CAD entities. Speaks `rig.cad.dimension`.
 * @details Not a solid. `measurement` true = label only. False = a solver may
 * move `a` / `b` to match `value`. `value` is a scene length except `Angle`
 * (degrees). `b` is unused for `Diameter`.
 */
struct CCadDimension {
	enum class Kind : int {
		Linear = 0,
		Aligned = 1,
		Horizontal = 2,
		Vertical = 3,
		Diameter = 4,
		Angle = 5
	};

	Kind kind = Kind::Linear;
	std::string a;
	std::string b;
	float value = 1.f;
	bool measurement = false;
	glm::vec3 offset{0.f, 0.f, 0.f};

	std::vector<sProp> GetProperties() {
		static constexpr const char* kKindNames[] = {"Linear",	 "Aligned",	 "Horizontal",
													 "Vertical", "Diameter", "Angle"};
		return {{0, "Kind", EPT_ENUM, reinterpret_cast<int*>(&kind), kKindNames, 6},
				{1, "A", EPT_STRING, &a},
				{2, "B", EPT_STRING, &b},
				{3, "Value", EPT_FLOAT, &value},
				{4, "Measurement", EPT_BOOL, &measurement},
				{5, "Offset", EPT_VEC3, &offset}};
	}
};

} // namespace ecs
} // namespace rigkit
