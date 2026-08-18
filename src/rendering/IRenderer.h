#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class Canvas;
class Graphics;

enum class RendererType { OpenGL, Blend2D };

enum class GradientType { Linear, Radial, Conic };

struct GradientStop {
	float offset = 0.f;
	glm::vec4 color{1.f};

	GradientStop(float o, const glm::vec4& c) : offset(o), color(c) {}
};

/**
 * @brief Colour plus fill-or-stroke intent for one draw call.
 * @details Every primitive takes a Paint, so the backend holds no style state
 * and stays free to reorder or batch draws. Retained artist style — set a
 * colour once, then draw many shapes — lives one layer up in Graphics.
 * @see Graphics, OpenGLRenderer
 */
struct Paint {
	enum class Mode { Fill, Stroke };

	Mode mode = Mode::Fill;
	glm::vec4 color{1.f};
	/** @brief Line width in design pixels; unused when mode is Fill. */
	float strokeWidth = 1.f;

	/** @brief Paint that fills with @p color. */
	static Paint fill(const glm::vec4& color) { return {Mode::Fill, color, 1.f}; }

	/** @brief Paint that strokes with @p color at @p width design pixels. */
	static Paint stroke(const glm::vec4& color, float width) {
		return {Mode::Stroke, color, width};
	}
};

/**
 * @brief Abstract 2D present backend (primitives, paths, style).
 * @details Default host fulfillment is OpenGLRenderer. Style is per call via
 * Paint; the matrix stack is the only retained drawing state. Gradients, pixel
 * export, and rich text may be no-ops until a pack (e.g. Blend2D) implements
 * them. Does not create a window or swap buffers.
 * @see OpenGLRenderer, createRenderer, Graphics, Paint
 */
class IRenderer {
  public:
	virtual ~IRenderer() = default;

	virtual bool initialize(int width, int height) = 0;
	virtual void shutdown() = 0;
	/** @brief Logical / design size (ortho / artist coordinates). */
	virtual void resize(int width, int height) = 0;
	/**
	 * @brief Physical framebuffer size for the present viewport (HiDPI).
	 * @details Defaults to the design size from resize(). OpenGL uses this for
	 * glViewport while ortho stays on design pixels.
	 */
	virtual void setFramebufferSize(int width, int height) {
		(void)width;
		(void)height;
	}

	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;
	virtual void clear(const glm::vec4& color) = 0;

	/** @note Always strokes — a line has no interior, so Paint::mode is ignored. */
	virtual void drawLine(float x1, float y1, float x2, float y2, const Paint& paint) = 0;
	virtual void drawRect(float x, float y, float width, float height, const Paint& paint) = 0;
	virtual void drawCircle(float x, float y, float radius, const Paint& paint) = 0;
	/**
	 * @brief Draw an ellipse.
	 * @param x Center x.
	 * @param y Center y.
	 * @param width Full width (not radius).
	 * @param height Full height.
	 * @param paint Fill or stroke style for this call.
	 */
	virtual void drawEllipse(float x, float y, float width, float height, const Paint& paint) = 0;
	virtual void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
							  const Paint& paint) = 0;
	virtual void drawPolygon(const std::vector<glm::vec2>& points, const Paint& paint) = 0;

	virtual void beginPath() = 0;
	virtual void moveTo(float x, float y) = 0;
	virtual void lineTo(float x, float y) = 0;
	virtual void curveTo(float cx1, float cy1, float cx2, float cy2, float x, float y) = 0;
	virtual void closePath() = 0;
	virtual void fill(const glm::vec4& color) = 0;
	virtual void stroke(const glm::vec4& color, float width) = 0;

	virtual void setFont(const std::string& fontPath, float size) = 0;
	virtual void drawText(const std::string& text, float x, float y, const glm::vec4& color) = 0;
	virtual glm::vec2 getTextBounds(const std::string& text) = 0;

	virtual void pushMatrix() = 0;
	virtual void popMatrix() = 0;
	virtual void translate(float x, float y) = 0;
	virtual void rotate(float angle) = 0;
	virtual void scale(float sx, float sy) = 0;
	virtual void resetMatrix() = 0;

	/** @note OpenGL default path may no-op; Blend2D pack can fulfill. */
	virtual void setLinearGradient(float x1, float y1, float x2, float y2,
								   const std::vector<GradientStop>& stops) = 0;
	virtual void setRadialGradient(float cx, float cy, float radius,
								   const std::vector<GradientStop>& stops) = 0;
	virtual void setConicGradient(float cx, float cy, float angle,
								  const std::vector<GradientStop>& stops) = 0;
	virtual void clearGradient() = 0;

	/** @note May return false when the backend has no pixel export. */
	virtual bool saveToFile(const std::string& filename) = 0;
	virtual bool saveToMemory(std::vector<uint8_t>& data) = 0;

	virtual RendererType getType() const = 0;
	virtual std::string getName() const = 0;
	virtual bool isInitialized() const = 0;
	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
	virtual uint8_t* getPixelBuffer() = 0;

	/**
	 * @brief Independent triangles, 3 vertices each, document order.
	 * @details Default walks drawTriangle. OpenGL batches as GL_TRIANGLES.
	 */
	virtual void drawTriangles(const std::vector<glm::vec2>& pts, const Paint& paint) {
		for (size_t i = 0; i + 2 < pts.size(); i += 3) {
			drawTriangle(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, pts[i + 2].x, pts[i + 2].y,
						 paint);
		}
	}

	/**
	 * @brief Independent segments, 2 vertices each.
	 * @details Default walks drawLine. OpenGL batches as GL_LINES.
	 */
	virtual void drawLines(const std::vector<glm::vec2>& pts, const Paint& paint) {
		for (size_t i = 0; i + 1 < pts.size(); i += 2) {
			drawLine(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, paint);
		}
	}
};

/**
 * @brief Create a renderer via RendererRegistry (nullptr if type unregistered).
 */
std::shared_ptr<IRenderer> createRenderer(RendererType type);
