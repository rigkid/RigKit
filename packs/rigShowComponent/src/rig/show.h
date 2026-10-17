#pragma once

/**
 * @file
 * @brief Creators for the show deck and analysis requests.
 */

#include <entt/entt.hpp>
#include <string>
#include "CVisualizer.h"

namespace rigkit {
class MEcs;
}

namespace rig {

/** @brief Visualizer deck (`CVisualizer`). Present packs draw into `output`. */
entt::entity makeVisualizer(const std::string& presetDir = "data/presets",
							const std::string& name = {});
entt::entity makeVisualizer(rigkit::MEcs& ecs, const std::string& presetDir = "data/presets",
							const std::string& name = {});

/** @brief Append a `kind` preset `CAssetRef` to the deck and return it. */
entt::entity addVisualizerPreset(entt::entity visualizer, const std::string& path,
								 const std::string& name = {});
entt::entity addVisualizerPreset(rigkit::MEcs& ecs, entt::entity visualizer,
								 const std::string& path, const std::string& name = {});

/** @brief Authored audio analysis (`CAudioAnalysis`). Live values stay on the bus. */
entt::entity makeAudioAnalysis(const std::string& name = {});
entt::entity makeAudioAnalysis(rigkit::MEcs& ecs, const std::string& name = {});

/** @brief Authored picture analysis (`CPictureAnalysis`) pointed at `source`. */
entt::entity makePictureAnalysis(entt::entity source, const std::string& name = {});
entt::entity makePictureAnalysis(rigkit::MEcs& ecs, entt::entity source,
								 const std::string& name = {});

} // namespace rig
