#pragma once

#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Modulator to property wire - speaks `rig.mod.binding`.
 * @details `source` / `target` are entity names (same as `createEntity` / `findEntity`).
 * `propertyKey` matches `GetProperties` names (and dotted aliases like `position.y`).
 * Advanced by `SModulators` each Update.
 */
struct CModBinding {
	std::string source;		 ///< Modulator entity name (e.g. LFO)
	std::string target;		 ///< Driven entity name
	std::string propertyKey; ///< Field on target
	float depth = 1.f;
	float min = 0.f;
	float max = 0.f;
	bool hasMin = false;
	bool hasMax = false;
	bool additive = false;

	std::vector<sProp> GetProperties() {
		return {{0, "source", EPT_STRING, &source},
				{1, "target", EPT_STRING, &target},
				{2, "propertyKey", EPT_STRING, &propertyKey},
				{3, "depth", EPT_FLOAT, &depth},
				{4, "additive", EPT_BOOL, &additive}};
	}
};

} // namespace ecs
} // namespace rigkit
