#pragma once

#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Drive one float property over time - speaks `rig.anim.tween`.
 * @details Advanced by `STweens` each Update via `writeEntityProperty`.
 */
struct CTween {
	std::string target;
	std::string propertyKey;
	float from = 0.f;
	float to = 1.f;
	float duration = 1.f;
	float elapsed = 0.f;
	std::string easing = "linear"; ///< linear | ease-in | ease-out | ease-in-out
	bool loop = false;
	bool playing = true;

	std::vector<sProp> GetProperties() {
		return {{0, "target", EPT_STRING, &target},	   {1, "propertyKey", EPT_STRING, &propertyKey},
				{2, "from", EPT_FLOAT, &from},		   {3, "to", EPT_FLOAT, &to},
				{4, "duration", EPT_FLOAT, &duration}, {5, "elapsed", EPT_FLOAT, &elapsed},
				{6, "easing", EPT_STRING, &easing},	   {7, "loop", EPT_BOOL, &loop},
				{8, "playing", EPT_BOOL, &playing}};
	}
};

} // namespace ecs
} // namespace rigkit
