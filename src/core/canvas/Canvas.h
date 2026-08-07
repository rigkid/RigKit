#pragma once

// Standard library
#include <memory>
#include <string>
#include <variant>
#include <vector>

// Third-party
#include <glm/glm.hpp>

// Project headers
#include "core/ISettings.h"
#include "rendering/IRenderer.h"

namespace rigkit {

// Forward declarations
class RigKitEngine;
class Graphics;

/**
 * @brief Size and clear-color defaults for Canvas construction.
 */
struct CanvasSettings {
	int width = 800;
	int height = 600;
	float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
	int samples = 0;
};

/**
 * @brief Off-screen drawing surface (FBO) with optional ECS present.
 * @details Immediate draws go through Graphics → IRenderer. Host window present
 * does not require a Canvas; use beginOffscreen/endOffscreen for FBO work.
 * Frame export / SVG live in renderer packs — not on this type.
 * @see Graphics, MCanvas, MEcs::renderSystems
 */
class Canvas : public ISettings {
  public:
	Canvas(const CanvasSettings& settings, RigKitEngine* engine = nullptr);
	~Canvas();

	void resize(int width, int height);
	void clear();
	void clear(float r, float g, float b, float a = 1.0f);
	void background(float r, float g, float b, float a = 1.0f);
	void fill(float r, float g, float b, float a = 1.0f);
	void noFill();
	void stroke(float r, float g, float b, float a = 1.0f);
	void noStroke();
	void strokeWeight(float weight);
	void rect(float x, float y, float width, float height);
	void ellipse(float x, float y, float width, float height);
	void line(float x1, float y1, float x2, float y2);
	void triangle(float x1, float y1, float x2, float y2, float x3, float y3);
	void circle(float x, float y, float diameter);
	void text(const std::string& text, float x, float y);
	void textSize(float size);
	void textAlign(int align);
	void pushMatrix();
	void popMatrix();
	void translate(float x, float y);
	void rotate(float angle);
	void scale(float x, float y);

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

	// Getters
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	std::shared_ptr<Graphics> getGraphics() { return m_graphics; }
	unsigned int getColorTexture() const;
	void setName(const std::string& name) { m_name = name; }
	std::string getName() const { return m_name; }

	// ISettings interface
	json getSettings() const override;
	void setSettings(const json& settings) override;

  private:
	void initFramebuffer();
	void initShaders();

	int m_width;
	int m_height;
	std::string m_name;
	CanvasSettings m_settings;

	// PIMPL for OpenGL resources
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	std::shared_ptr<Graphics> m_graphics;
	std::shared_ptr<IRenderer> m_ownedRenderer;

	float m_textSize = 12.f;
	int m_textAlign = 0;

	float m_frameRate = 60.f;

	RigKitEngine* m_engine = nullptr;
};

} // namespace rigkit
