#pragma once

#include <vector>

namespace rigkit {
namespace ecs {

/**
 * @brief Axis variation map on the same entity as `CFontAxis`. Speaks `rig.font.avar`.
 */
struct CFontAvar {
	struct Segment {
		float fromCoord = 0.f;
		float toCoord = 0.f;
	};

	std::vector<Segment> segments;

	static constexpr bool kNoProperties = true;
};

} // namespace ecs
} // namespace rigkit
