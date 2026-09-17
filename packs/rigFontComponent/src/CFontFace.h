#pragma once

#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Editable typeface metrics. Speaks `rig.font.face`.
 * @details UFO source face. Contours live on glyph entities as `CPath`.
 * `features` is a document / entity name pointing at a `CCode` `fea` buffer.
 */
struct CFontFace {
	std::string family;
	std::string style = "Regular";
	float unitsPerEm = 1000.f;
	float ascender = 800.f;
	float descender = -200.f;
	float capHeight = 700.f;
	float xHeight = 500.f;
	std::string version;
	std::string features;

	std::vector<sProp> GetProperties() {
		return {
			{0, "Family", EPT_STRING, &family},			 {1, "Style", EPT_STRING, &style},
			{2, "Units Per Em", EPT_FLOAT, &unitsPerEm}, {3, "Ascender", EPT_FLOAT, &ascender},
			{4, "Descender", EPT_FLOAT, &descender},	 {5, "Cap Height", EPT_FLOAT, &capHeight},
			{6, "X Height", EPT_FLOAT, &xHeight},		 {7, "Version", EPT_STRING, &version},
			{8, "Features", EPT_STRING, &features}};
	}
};

} // namespace ecs
} // namespace rigkit
