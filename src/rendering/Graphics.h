#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "rendering/IRenderer.h"

namespace rigkit {

/**
 * @brief Thin façade over an IRenderer for immediate-mode drawing.
 * @details Holds the retained artist style - set a colour once, then draw many
 * shapes - and turns it into an explicit Paint per call, because IRenderer
 * keeps no style state. This is the one owner of that style: callers above
 * (Canvas, author/rigDraw.h) forward here rather than caching their own copy.
 * Each draw fills first and strokes second, so a shape can do both. Does not
 * implement shadows or image loading. Stroke joins and caps live on the
 * present renderer (OpenGL default: round).
 * @see IRenderer, Paint, OpenGLRenderer, author/rigDraw.h
 */
class Graphics {
  public:
	Graphics();
	~Graphics();

	void setRenderer(IRenderer* renderer);
	IRenderer* getRenderer() const { return m_renderer; }

	/** @brief Set the fill colour and enable filling. */
	void setFillColor(float r, float g, float b, float a = 1.0f);
	/** @brief Stop filling subsequent shapes. */
	void noFill();
	/** @brief Set the stroke colour and enable stroking. */
	void setStrokeColor(float r, float g, float b, float a = 1.0f);
	/** @brief Stop stroking subsequent shapes. */
	void noStroke();
	/** @brief Stroke width in design pixels; does not enable or disable stroking. */
	void setStrokeWidth(float width);
	/** @brief Scale applied to fill alpha, for fading a group of shapes. */
	void setFillOpacity(float opacity);

	void drawRect(float x, float y, float width, float height);
	void drawEllipse(float x, float y, float width, float height);
	void drawCircle(float x, float y, float radius);
	void drawLine(float x1, float y1, float x2, float y2);
	void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3);
	void drawPolygon(const std::vector<glm::vec2>& points);

	void beginPath();
	void moveTo(float x, float y);
	void lineTo(float x, float y);
	void curveTo(float x1, float y1, float x2, float y2, float x3, float y3);
	void closePath();
	void fill();
	void stroke();

	/** @brief Placeholder until a font pack; draws a bar sized by text length. */
	void drawText(const std::string& text, float x, float y);
	void setFont(const std::string& fontName, float size);
	void setTextAlign(int align);

	void pushMatrix();
	void popMatrix();
	void translate(float x, float y);
	void rotate(float angle);
	void scale(float x, float y);

	void setLinearGradient(float x1, float y1, float x2, float y2,
						   const std::vector<GradientStop>& stops);
	void setRadialGradient(float cx, float cy, float radius,
						   const std::vector<GradientStop>& stops);
	void setConicGradient(float cx, float cy, float angle, const std::vector<GradientStop>& stops);
	void clearGradient();

	void clear(float r, float g, float b, float a = 1.0f);
	void save();
	void restore();
	void setCanvasSize(int width, int height);

	glm::vec4 getFillColor() const { return m_fillColor; }
	glm::vec4 getStrokeColor() const { return m_strokeColor; }
	float getStrokeWidth() const { return m_strokeWidth; }
	bool hasFill() const { return m_hasFill; }
	bool hasStroke() const { return m_hasStroke; }

  private:
	/** @brief Current fill style, with fill opacity folded into alpha. */
	Paint fillPaint() const;
	Paint strokePaint() const;

	glm::vec4 m_fillColor{1.f, 1.f, 1.f, 1.f};
	glm::vec4 m_strokeColor{0.f, 0.f, 0.f, 1.f};
	float m_strokeWidth = 1.f;
	float m_fillOpacity = 1.f;
	bool m_hasFill = true;
	bool m_hasStroke = true;

	std::vector<glm::vec2> m_currentPath;
	bool m_pathOpen = false;

	std::string m_currentFont;
	float m_fontSize = 12.f;
	int m_textAlign = 0;

	IRenderer* m_renderer = nullptr;
	int m_canvasWidth = 0;
	int m_canvasHeight = 0;
};

} // namespace rigkit
