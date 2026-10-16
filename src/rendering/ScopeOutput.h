#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace rigkit {

/**
 * @brief One polyline on a scope layer, in that layer's pixel frame (Y down).
 */
struct ScopeContour {
	std::vector<glm::vec2> pts;
};

/**
 * @brief Named picture on the XY scope output.
 * @details Same shape as a compositor layer: an id, a frame, and the strokes
 * that layer contributes. Publishers replace their id when the picture changes
 * and remove it when the picture is gone. rigAudio's XY scope traces every
 * layer that is present while the output is on (Preferences > Audio > Playback
 * > XY Scope, or `rigAudio::setXyScope`).
 *
 * "plot" is the pixel document's mask, region, and fill strokes. "chrome" is
 * the character-grid monitor (VarFont outlines plus box-drawing segments).
 * "text" is `CText`. "imgui" is ImGui window frames and titles when that chrome
 * is up.
 */
struct ScopeLayer {
	std::string id;
	float frameW = 0.f;
	float frameH = 0.f;
	std::vector<ScopeContour> contours;
	/**
	 * @brief Fingerprint of `contours`. 0 means the publisher did not hash them,
	 * so the scope must recopy. A non-zero match can skip the trace rebuild.
	 */
	uint64_t hash = 0;
};

/**
 * @brief Replace the layer with this id. Empty id is ignored.
 * @details Main thread. Does not allocate the audio trace; the scope device
 * copies layers while it is on.
 */
void publishScopeLayer(ScopeLayer layer);

/** @brief Drop a layer. No-op when the id is not present. */
void removeScopeLayer(const std::string& id);

/** @brief Layers currently published, in registration order. */
const std::vector<ScopeLayer>& scopeLayers();

/**
 * @brief True while the XY scope device is open.
 * @details Publishers skip outline work when this is false.
 */
void setScopeOutputEnabled(bool enabled);
bool scopeOutputEnabled();

/**
 * @brief True while the XY test circle is up.
 * @details Publishers skip plot work so the circle is the only figure.
 */
void setScopeOutputCalibrating(bool on);
bool scopeOutputCalibrating();

} // namespace rigkit
