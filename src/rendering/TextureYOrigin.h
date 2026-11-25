#pragma once

/**
 * @file TextureYOrigin.h
 * @brief Present metadata: which edge of a color attachment is texel v=0.
 * @details RigKit owns this direction so packs do not invent V-flips. 2D Canvas
 * / OpenGLRenderer writes top-left (Y down). `SMeshPresent3D` into a texture /
 * FBO stays GL bottom-left - present with V-only UVs. Window framebuffer is
 * also bottom-left. ImGui blits and picks derive UVs from this value only.
 */

#include <cstdint>

namespace rigkit {

/** @brief Where v=0 sits on a color texture destined for screen present. */
enum class TextureYOrigin : uint8_t {
	BottomLeft = 0, ///< Raw GL FBO / offscreen SMeshPresent3D / window
	TopLeft = 1,	///< Canvas 2D, CPU upload (ImGui-native)
};

/**
 * @brief ImGui-style UVs: widget top-left samples uv0, bottom-right uv1.
 * @details Not a second scene graph - only present sampling.
 */
struct PresentUv {
	float u0 = 0.f;
	float v0 = 0.f;
	float u1 = 1.f;
	float v1 = 1.f;
};

/**
 * @brief UVs so a screen top-left widget shows the attachment upright.
 * @param origin Writer convention of the color attachment.
 * @param uMax Right edge of the used subrect in [0,1] (viewport may be a corner of a larger FBO).
 * @param vMax Top edge of the used subrect in GL v (v=0 is attachment bottom).
 */
inline PresentUv presentUvForScreenTopLeft(TextureYOrigin origin, float uMax = 1.f,
										   float vMax = 1.f) {
	if (origin == TextureYOrigin::BottomLeft) {
		// V-only within the used subrect.
		return {0.f, vMax, uMax, 0.f};
	}
	return {0.f, 0.f, uMax, vMax};
}

/**
 * @brief Map widget-local Y (0 at top, grows down) to NDC Y for picking.
 * @details Matches `presentUvForScreenTopLeft`: BottomLeft needs the usual
 * ImGui-to-NDC flip; TopLeft maps widget top to NDC -Y (unflipped blit).
 */
inline float presentNdcYFromWidgetLocal(TextureYOrigin origin, float localY, float widgetH) {
	if (widgetH <= 1.f) {
		return 0.f;
	}
	const float t = localY / widgetH;
	if (origin == TextureYOrigin::BottomLeft) {
		return 1.f - 2.f * t;
	}
	return 2.f * t - 1.f;
}

/**
 * @brief True when grab-pan / tumble should treat camera up as widget up.
 * @details BottomLeft + V-flip blit: ImGui mouseDy (screen-down) opposes camera
 * up. TopLeft unflipped: camera up matches widget up.
 */
inline bool presentMouseYMatchesCameraUp(TextureYOrigin origin) {
	return origin == TextureYOrigin::TopLeft;
}

} // namespace rigkit
