#pragma once

#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace stage {

/**
 * @brief Spawn recipe. Speaks nothing portable yet (app data).
 * @details The entity is a recipe, not a body: no `CRigidbody`, so **rigJolt**
 * leaves it alone. `stage.spawn` reads `name` and writes a dynamic sphere.
 */
struct CStagePrefab {
	std::string name;
	float radius = 0.35f;
	float mass = 1.f;
	float colorR = 0.90f;
	float colorG = 0.35f;
	float colorB = 0.25f;

	std::vector<sProp> GetProperties() {
		return {{0, "Name", EPT_STRING, &name},		{1, "Radius", EPT_FLOAT, &radius},
				{2, "Mass", EPT_FLOAT, &mass},		{3, "Color R", EPT_FLOAT, &colorR},
				{4, "Color G", EPT_FLOAT, &colorG}, {5, "Color B", EPT_FLOAT, &colorB}};
	}
};

} // namespace stage
