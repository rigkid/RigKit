#pragma once

#include <memory>
#include <string>

#include "core/ISettings.h"
#include "rendering/IRenderer.h"

namespace rigkit {

class RigKitEngine;
class Graphics;

/**
 * @brief Size, clear color, and FBO MSAA for Canvas construction.
 * @details `samples` of -1 inherits `window().samples`. 0 is off.
 */
struct CanvasSettings {
	int width = 800;
	int height = 600;
	float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
	/// FBO MSAA. -1 inherits `window().samples`. 0 is off. >0 is that count.
	int samples = -1;
};

/**
 * @brief Off-screen drawing surface (FBO) with optional ECS present.
 * @details Host window present does not require a Canvas; use
 * beginOffscreen/endOffscreen for FBO work. Artist shapes go through `rig::`
 * sketch or creators, not this type. Frame export / SVG live in renderer packs.
 * @see Graphics, MCanvas, MEcs::renderSystems
 */
class Canvas : public ISettings {
  public:
	Canvas(const CanvasSettings& settings, RigKitEngine* engine = nullptr);
	~Canvas();

	void resize(int width, int height);
	void clear();
	void clear(float r, float g, float b, float a = 1.0f);

	/**
	 * @brief Bind / unbind the canvas FBO for off-screen present.
	 * @details ECS shapes reach this FBO through the SCanvasRender Draw system
	 * in rigSystems, not from here.
	 */
	void beginOffscreen();
	void endOffscreen();

	RigKitEngine* getEngine() const { return m_engine; }

	void switchRenderer(RendererType type);
	void setRenderer(std::shared_ptr<IRenderer> renderer);
	RendererType getRendererType() const;

	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	std::shared_ptr<Graphics> getGraphics() { return m_graphics; }
	unsigned int getColorTexture() const;
	void setName(const std::string& name) { m_name = name; }
	std::string getName() const { return m_name; }

	json getSettings() const override;
	void setSettings(const json& settings) override;

  private:
	void destroyFramebuffer();
	void initFramebuffer();
	void resolveIfNeeded() const;
	int requestedSamples() const;
	void initShaders();

	int m_width;
	int m_height;
	std::string m_name;
	CanvasSettings m_settings;

	struct Impl;
	std::unique_ptr<Impl> m_impl;

	std::shared_ptr<Graphics> m_graphics;
	std::shared_ptr<IRenderer> m_ownedRenderer;

	RigKitEngine* m_engine = nullptr;
};

} // namespace rigkit
