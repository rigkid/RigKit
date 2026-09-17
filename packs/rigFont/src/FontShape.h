#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ecs/MEcs.h"
#include "CPath.h"

namespace rigkit {
namespace font {

struct ShapedGlyph {
	entt::entity entity{entt::null};
	std::string name;
	float x = 0.f;
	float advance = 0.f;
	ecs::CPath path;
};

struct ShapedRun {
	std::vector<ShapedGlyph> glyphs;
	float width = 0.f;
};

/**
 * @brief Map UTF-8 to source glyphs, advance + optional kern, flatten composites.
 * @details @p location empty uses the default `CPath`. Non-empty reconstructs cells.
 */
ShapedRun shapeString(MEcs& ecs, std::string_view text, bool applyKern,
					  const std::unordered_map<std::string, float>& location = {});

/**
 * @brief Reconstruct one glyph outline at @p location (`tag` to design value).
 * @return false when the glyph has no path.
 */
bool reconstructGlyph(MEcs& ecs, entt::entity glyph,
					  const std::unordered_map<std::string, float>& location, ecs::CPath& outPath,
					  float& outAdvance);

/**
 * @brief Write one linear cell term from a topology-matched other path.
 */
bool writeCellFromPathDiff(MEcs& ecs, entt::entity glyph, const ecs::CPath& other,
						   const std::string& axisTag, int sign = 1);

} // namespace font
} // namespace rigkit
