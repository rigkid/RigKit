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
	/// Slant in degrees, counter-clockwise from vertical (UFO italicAngle).
	float italicAngle = 0.f;
	std::string version;
	std::string copyright;
	std::string trademark;
	std::string designer;
	std::string designerUrl;
	std::string manufacturer;
	std::string manufacturerUrl;
	std::string licence;
	std::string licenceUrl;
	std::string note;
	std::string features;

	std::vector<sProp> GetProperties() {
		return {{0, "Family", EPT_STRING, &family},
				{1, "Style", EPT_STRING, &style},
				{2, "Units Per Em", EPT_FLOAT, &unitsPerEm},
				{3, "Ascender", EPT_FLOAT, &ascender},
				{4, "Descender", EPT_FLOAT, &descender},
				{5, "Cap Height", EPT_FLOAT, &capHeight},
				{6, "X Height", EPT_FLOAT, &xHeight},
				{7, "Italic Angle", EPT_FLOAT, &italicAngle},
				{8, "Version", EPT_STRING, &version},
				{9, "Copyright", EPT_STRING, &copyright},
				{10, "Trademark", EPT_STRING, &trademark},
				{11, "Designer", EPT_STRING, &designer},
				{12, "Designer URL", EPT_STRING, &designerUrl},
				{13, "Manufacturer", EPT_STRING, &manufacturer},
				{14, "Manufacturer URL", EPT_STRING, &manufacturerUrl},
				{15, "Licence", EPT_STRING, &licence},
				{16, "Licence URL", EPT_STRING, &licenceUrl},
				{17, "Note", EPT_STRING, &note},
				{18, "Features", EPT_STRING, &features}};
	}
};

} // namespace ecs
} // namespace rigkit
