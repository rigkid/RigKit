#pragma once

#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Kerning class. Speaks `rig.font.group`.
 * @details Name is `CName.stableId` (`public.kern1.A`). Members are glyph names.
 */
struct CFontGroup {
	enum Side { Unclassified = 0, Left, Right };

	std::vector<std::string> members;
	int side = Unclassified;

	static constexpr const char* kSideNames[] = {"unclassified", "left", "right"};
	static constexpr int kSideCount = 3;

	std::vector<sProp> GetProperties() {
		return {{0, "Side", EPT_ENUM, &side, kSideNames, kSideCount}};
	}
};

} // namespace ecs
} // namespace rigkit
