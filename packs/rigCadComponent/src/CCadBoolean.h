#pragma once

#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief CSG combination of solids. Speaks `rig.cad.boolean`.
 * @details Difference is `operands[0]` minus the rest. Operand strings are entity names.
 */
struct CCadBoolean {
	enum class Op : int { Union = 0, Difference = 1, Intersection = 2 };

	Op op = Op::Difference;
	std::vector<std::string> operands;

	int propOperandCount = 0;

	std::vector<sProp> GetProperties() {
		static constexpr const char* kOpNames[] = {"Union", "Difference", "Intersection"};
		propOperandCount = static_cast<int>(operands.size());
		return {{0, "Op", EPT_ENUM, reinterpret_cast<int*>(&op), kOpNames, 3},
				{1, "Operands", EPT_INT, &propOperandCount}};
	}
};

} // namespace ecs
} // namespace rigkit
