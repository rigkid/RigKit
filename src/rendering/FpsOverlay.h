#pragma once

class IRenderer;

namespace rigkit {

/**
 * @brief Corner FPS readout on the host 2D present path.
 * @details Call after Draw systems and before `IRenderer::endFrame`. Uses the
 * engine clock (`instantFps` / `dt`); does not read GLFW. No-op when `fps` is 0.
 * Uses `IRenderer` filled text when a font pack (rigVarFont) is attached;
 * otherwise 3x5 bitmap digits.
 */
void presentFpsOverlay(IRenderer& renderer, int instantFps, float dt);

} // namespace rigkit
