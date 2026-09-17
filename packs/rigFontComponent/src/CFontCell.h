#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Lattice deltas on a glyph. Speaks `rig.font.cell`.
 * @details Default outline stays on `CPath`. Terms reconstruct other locations.
 */
struct CFontCell {
	struct DeltaSeg {
		enum Type { Line = 0, Quad };
		int type = Line;
		std::vector<glm::vec2> points;
	};

	struct Term {
		std::string axisTag;
		int sign = 1;
		std::string axisTag2;
		int sign2 = 1;
		std::string axisTag3;
		int sign3 = 1;
		float center = 0.f;
		float wl = 0.f;
		float wr = 0.f;
		bool hasHat = false;
		float adv = 0.f;
		std::vector<DeltaSeg> segs;
	};

	float baseAdv = 0.f;
	std::vector<Term> terms;

	std::vector<sProp> GetProperties() { return {{0, "Base Advance", EPT_FLOAT, &baseAdv}}; }
};

} // namespace ecs
} // namespace rigkit
