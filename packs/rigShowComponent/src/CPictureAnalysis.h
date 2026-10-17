#pragma once

#include <entt/entt.hpp>
#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Authored picture analysis request. Speaks `rig.picture.analysis`.
 * @details `source` is a canvas, raster, image asset, or visualizer output.
 * Live color / temperature stay on the picture bus. A present pack
 * reads `source` pixels, then `rig::node::publishPictureFrame`.
 */
struct CPictureAnalysis {
	entt::entity source{entt::null};
	std::vector<std::string> features;
	int propFeatureCount = 0;

	std::vector<sProp> GetProperties() {
		propFeatureCount = static_cast<int>(features.size());
		return {{0, "features", EPT_INT, &propFeatureCount}};
	}
};

} // namespace ecs
} // namespace rigkit
