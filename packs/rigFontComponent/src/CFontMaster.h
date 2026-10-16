#pragma once

#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief One interpolation master. Speaks `rig.font.master`.
 * @details Composes on the `CFontLayer` entity holding this master's outlines
 * (the designspace source idea). Axes absent from `location` sit at their
 * default. Location is variable-length, so the Layers panel edits it, not the
 * generic inspector.
 */
struct CFontMaster {
	static constexpr bool kNoProperties = true;

	struct AxisValue {
		std::string tag;
		float value = 0.f;
	};

	std::vector<AxisValue> location;

	/** @brief Design value for @p tag, or @p fallback when absent. */
	float valueFor(const std::string& tag, float fallback = 0.f) const {
		for (const auto& av : location) {
			if (av.tag == tag) {
				return av.value;
			}
		}
		return fallback;
	}

	/** @brief Set or append the design value for @p tag. */
	void setValue(const std::string& tag, float value) {
		for (auto& av : location) {
			if (av.tag == tag) {
				av.value = value;
				return;
			}
		}
		location.push_back({tag, value});
	}
};

} // namespace ecs
} // namespace rigkit
