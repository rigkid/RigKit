#pragma once

#include <string>

#include "ecs/MEcs.h"
#include "ComponentSerializerRegistry.h"

namespace rigkit {
namespace ufo {

/**
 * @brief Load a Contract font document, write UFO, read back, compare glyphs/kern.
 * @return true when face, glyph A path, and kern AV survive the trip.
 */
bool roundtripFromContractJson(MEcs& ecs, const project::ComponentSerializerRegistry& codecs,
							   const std::string& jsonPath, const std::string& ufoDir,
							   std::string* error = nullptr);

} // namespace ufo
} // namespace rigkit
