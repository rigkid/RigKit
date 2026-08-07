#pragma once

#include <entt/entt.hpp>
#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {

/** @brief Thin host binding for a data component type (not component logic). */
struct ComponentTypeInfo {
	std::string name;
	bool portable = true;
	bool (*has)(entt::registry&, entt::entity) = nullptr;
	std::vector<sProp> (*properties)(entt::registry&, entt::entity) = nullptr;
};

} // namespace rigkit
