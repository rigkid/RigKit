#pragma once

#include <vector>
#include "core/json.h"
#include "ecs/PropertyReflection.h"

namespace rigkit {

/// Serialize live property descriptors into a JSON object (keys = prop names).
json propsToJson(const std::vector<sProp>& props);

/// Apply JSON object fields onto live property pointers. Missing keys leave values unchanged.
void jsonToProps(const json& j, std::vector<sProp>& props);

} // namespace rigkit
