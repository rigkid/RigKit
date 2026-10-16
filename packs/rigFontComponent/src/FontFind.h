#pragma once

#include <entt/entt.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "ecs/MEcs.h"

namespace rigkit {
namespace font {

/**
 * @brief Entity whose name or `CName.stableId` matches @p id.
 * @return The first match, or `entt::null`.
 */
entt::entity findNamed(MEcs& ecs, const std::string& id);

/**
 * @brief Rewrite Contract document ids on font refs to live entity names.
 * @details After `importContractJson` with codecs, `CFontFace.features` and
 * `CFontComponent.glyph` still hold document ids (`glyph-A`). This maps them
 * to `entityName` so later lookups use `findNamed`.
 */
void rewriteImportedRefs(MEcs& ecs, const std::unordered_map<std::string, entt::entity>& docIds);

/** @brief Ensure glyphs / layers / faces carry `CName` from the entity name. */
void ensureNames(MEcs& ecs);

/** @brief `CName.stableId`, else entity name. */
std::string stableName(MEcs& ecs, entt::entity e);

/**
 * @brief UFO pair lookup: exact, then group-to-glyph / glyph-to-group / group-to-group.
 * @return 0 when no pair exists.
 */
float kernValue(MEcs& ecs, const std::string& left, const std::string& right);

/** @brief Exact `CFontKern` entity for these names, or `entt::null`. */
entt::entity findKernEntity(MEcs& ecs, const std::string& left, const std::string& right);

/**
 * @brief Write or replace one pair parented to the first face.
 * @details Value 0 deletes the pair (empty cell).
 */
void setKernPair(MEcs& ecs, const std::string& left, const std::string& right, float value);

/** @brief First glyph whose `unicodes` contains @p cp, else `.notdef`, else null. */
entt::entity findGlyphByUnicode(MEcs& ecs, uint32_t cp);

/** @brief Layer entities parented to @p face, in registry order. */
std::vector<entt::entity> layersOfFace(MEcs& ecs, entt::entity face);

/** @brief Glyph named @p name whose parent is @p layer, or `entt::null`. */
entt::entity findGlyphInLayer(MEcs& ecs, entt::entity layer, const std::string& name);

} // namespace font
} // namespace rigkit
