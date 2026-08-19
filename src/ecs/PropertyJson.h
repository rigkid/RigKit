#pragma once

#include <vector>

#include "core/json.h"
#include "ecs/PropertyReflection.h"

namespace rigkit {

/// Serialize live property descriptors into a JSON object (keys = prop names).
/// Leaf encoding is `core/TypeJson.h` (`typeValueToJson`).
json propsToJson(const std::vector<sProp>& props);

/// Apply JSON object fields onto live property pointers. Missing keys leave values unchanged.
/// Leaf decoding is `typeValueFromJson`.
void jsonToProps(const json& j, std::vector<sProp>& props);

} // namespace rigkit
