#pragma once

#include <algorithm>
#include <glm/glm.hpp>

namespace rigkit {

/**
 * @brief Pan/zoom state for a 2D content view (no UI types).
 * @details screen = origin + content * zoomAbs. Content units are app-defined
 * (px, mm, …). Drivers set canvas bounds each frame, then call updateDerived().
 */
struct View2D {
	glm::vec2 contentSize{200.f, 200.f};
	float zoomMin = 0.1f;
	float zoomMax = 50.f;

	glm::vec2 pan{0.f, 0.f};
	float zoom = 1.f; ///< multiplier on fit zoom

	glm::vec2 canvasOrigin{0.f, 0.f};
	float canvasW = 1.f;
	float canvasH = 1.f;

	float ox = 0.f;
	float oy = 0.f;
	float zoomAbs = 1.f;
	bool hovered = false;

	float fitZoom() const {
		if (contentSize.x <= 0.f || contentSize.y <= 0.f || canvasW <= 0.f || canvasH <= 0.f) {
			return 1.f;
		}
		return (std::min)(canvasW / contentSize.x, canvasH / contentSize.y);
	}

	void updateDerived() {
		const float fit = fitZoom();
		zoomAbs = fit * zoom;
		if (zoomAbs < 1e-6f) {
			zoomAbs = 1e-6f;
		}
		const float contentW = contentSize.x * zoomAbs;
		const float contentH = contentSize.y * zoomAbs;
		ox = canvasOrigin.x + (canvasW - contentW) * 0.5f + pan.x;
		oy = canvasOrigin.y + (canvasH - contentH) * 0.5f + pan.y;
	}

	void fitToCanvas() {
		pan = {0.f, 0.f};
		zoom = 1.f;
		updateDerived();
	}

	void applyPanDelta(float dx, float dy) {
		pan.x += dx;
		pan.y += dy;
		updateDerived();
	}

	void applyScrollZoom(float scrollDelta, float pivotSx, float pivotSy) {
		updateDerived();
		const glm::vec2 contentUnder = toContent(pivotSx, pivotSy);
		zoom *= (scrollDelta > 0.f) ? 1.1f : (1.f / 1.1f);
		zoom = (std::clamp)(zoom, zoomMin, zoomMax);
		updateDerived();
		const glm::vec2 screenAfter = toScreen(contentUnder.x, contentUnder.y);
		pan.x += pivotSx - screenAfter.x;
		pan.y += pivotSy - screenAfter.y;
		updateDerived();
	}

	glm::vec2 toScreen(float cx, float cy) const { return {ox + cx * zoomAbs, oy + cy * zoomAbs}; }

	glm::vec2 toContent(float sx, float sy) const {
		if (zoomAbs <= 0.f) {
			return {};
		}
		return {(sx - ox) / zoomAbs, (sy - oy) / zoomAbs};
	}
};

} // namespace rigkit
