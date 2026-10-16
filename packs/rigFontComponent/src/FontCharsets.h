#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ecs/MEcs.h"

namespace rigkit {
namespace font {

/** @brief One named Unicode block the glyph list can group and generate. */
struct Charset {
	const char* name;
	uint32_t first;
	uint32_t last;
};

/** @brief Blocks the font tools know, in display order (Latin Basic first). */
const std::vector<Charset>& charsets();

/** @brief Index into `charsets()` whose range covers @p cp, or -1. */
int charsetIndex(uint32_t cp);

/** @brief AGL glyph name for printable ASCII, else `uniXXXX`. */
std::string glyphNameForCodepoint(uint32_t cp);

/**
 * @brief Create the missing glyphs of one block on one layer.
 * @details Skips codepoints and names the layer already has. New glyphs get
 * half-em width and parent to @p layer; `entt::null` targets the foreground
 * layer (first layer when no foreground exists).
 * @return Number of glyphs created.
 */
int generateCharset(MEcs& ecs, const Charset& cs, entt::entity layer = entt::null);

} // namespace font
} // namespace rigkit
