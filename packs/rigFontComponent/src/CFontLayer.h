#pragma once

#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief UFO layer identity. Speaks `rig.font.layer`.
 * @details Name lives on `CName` (`public.default`). Not `CLayer` (scene stack).
 */
struct CFontLayer {
	enum Role { Foreground = 0, Background, Spare };

	int role = Foreground;

	static constexpr const char* kRoleNames[] = {"foreground", "background", "spare"};
	static constexpr int kRoleCount = 3;

	std::vector<sProp> GetProperties() {
		return {{0, "Role", EPT_ENUM, &role, kRoleNames, kRoleCount}};
	}
};

} // namespace ecs
} // namespace rigkit
