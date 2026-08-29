#include "rendering/Graphics.h"
#include "rendering/U_gladGlfw.h"

namespace rigkit {

Graphics::Graphics() = default;
Graphics::~Graphics() = default;

void Graphics::setFillColor(float r, float g, float b, float a) {
	m_fillColor = glm::vec4(r, g, b, a);
	m_hasFill = true;
}

void Graphics::noFill() {
	m_hasFill = false;
}

void Graphics::setStrokeColor(float r, float g, float b, float a) {
	m_strokeColor = glm::vec4(r, g, b, a);
	m_hasStroke = true;
}

void Graphics::noStroke() {
	m_hasStroke = false;
}

void Graphics::setStrokeWidth(float width) {
	m_strokeWidth = width;
}

void Graphics::setFillOpacity(float opacity) {
	m_fillOpacity = opacity;
}

Paint Graphics::fillPaint() const {
	return Paint::fill(
		glm::vec4(m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a * m_fillOpacity));
}

Paint Graphics::strokePaint() const {
	return Paint::stroke(m_strokeColor, m_strokeWidth);
}

void Graphics::drawRect(float x, float y, float width, float height) {
	if (!m_renderer) {
		return;
	}
	if (m_hasFill) {
		m_renderer->drawRect(x, y, width, height, fillPaint());
	}
	if (m_hasStroke) {
		m_renderer->drawRect(x, y, width, height, strokePaint());
	}
}

void Graphics::drawEllipse(float x, float y, float width, float height) {
	if (!m_renderer) {
		return;
	}
	if (m_hasFill) {
		m_renderer->drawEllipse(x, y, width, height, fillPaint());
	}
	if (m_hasStroke) {
		m_renderer->drawEllipse(x, y, width, height, strokePaint());
	}
}

void Graphics::drawCircle(float x, float y, float radius) {
	if (!m_renderer) {
		return;
	}
	if (m_hasFill) {
		m_renderer->drawCircle(x, y, radius, fillPaint());
	}
	if (m_hasStroke) {
		m_renderer->drawCircle(x, y, radius, strokePaint());
	}
}

void Graphics::drawLine(float x1, float y1, float x2, float y2) {
	// A line has no interior, so fill style never applies.
	if (m_renderer && m_hasStroke) {
		m_renderer->drawLine(x1, y1, x2, y2, strokePaint());
	}
}

void Graphics::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
	if (!m_renderer) {
		return;
	}
	if (m_hasFill) {
		m_renderer->drawTriangle(x1, y1, x2, y2, x3, y3, fillPaint());
	}
	if (m_hasStroke) {
		m_renderer->drawTriangle(x1, y1, x2, y2, x3, y3, strokePaint());
	}
}

void Graphics::drawPolygon(const std::vector<glm::vec2>& points) {
	if (!m_renderer) {
		return;
	}
	if (m_hasFill) {
		m_renderer->drawPolygon(points, fillPaint());
	}
	if (m_hasStroke) {
		m_renderer->drawPolygon(points, strokePaint());
	}
}

void Graphics::beginPath() {
	m_currentPath.clear();
	m_pathOpen = true;
}

void Graphics::moveTo(float x, float y) {
	if (m_pathOpen) {
		m_currentPath.emplace_back(x, y);
	}
}

void Graphics::lineTo(float x, float y) {
	if (m_pathOpen) {
		m_currentPath.emplace_back(x, y);
	}
}

void Graphics::curveTo(float, float, float, float, float x3, float y3) {
	// Path builder: treat as line-to end point (no cubic tessellation).
	lineTo(x3, y3);
}

void Graphics::closePath() {
	if (m_pathOpen && !m_currentPath.empty()) {
		m_currentPath.push_back(m_currentPath[0]);
		m_pathOpen = false;
	}
}

void Graphics::fill() {
	if (m_renderer && !m_currentPath.empty()) {
		m_renderer->drawPolygon(m_currentPath, fillPaint());
	}
}

void Graphics::stroke() {
	if (!m_renderer || m_currentPath.size() < 2) {
		return;
	}
	const Paint paint = strokePaint();
	size_t n = m_currentPath.size();
	const bool closed = !m_pathOpen && n >= 3;
	if (closed && glm::length(m_currentPath.front() - m_currentPath.back()) < 1e-4f) {
		--n;
	}
	m_renderer->beginPath();
	m_renderer->moveTo(m_currentPath[0].x, m_currentPath[0].y);
	for (size_t i = 1; i < n; ++i) {
		m_renderer->lineTo(m_currentPath[i].x, m_currentPath[i].y);
	}
	if (closed) {
		m_renderer->closePath();
	}
	m_renderer->stroke(paint.color, paint.strokeWidth);
}

void Graphics::drawText(const std::string& text, float x, float y) {
	if (m_renderer) {
		m_renderer->drawText(text, x, y, m_fillColor);
	}
}

void Graphics::setFont(const std::string& fontName, float size) {
	m_currentFont = fontName;
	m_fontSize = size;
	if (m_renderer) {
		m_renderer->setFont(fontName, size);
	}
}

void Graphics::setTextAlign(int align) {
	m_textAlign = align;
}

void Graphics::pushMatrix() {
	if (m_renderer) {
		m_renderer->pushMatrix();
	}
}

void Graphics::popMatrix() {
	if (m_renderer) {
		m_renderer->popMatrix();
	}
}

void Graphics::translate(float x, float y) {
	if (m_renderer) {
		m_renderer->translate(x, y);
	}
}

void Graphics::rotate(float angle) {
	if (m_renderer) {
		m_renderer->rotate(angle);
	}
}

void Graphics::scale(float x, float y) {
	if (m_renderer) {
		m_renderer->scale(x, y);
	}
}

void Graphics::setLinearGradient(float x1, float y1, float x2, float y2,
								 const std::vector<GradientStop>& stops) {
	if (m_renderer) {
		m_renderer->setLinearGradient(x1, y1, x2, y2, stops);
	}
}

void Graphics::setRadialGradient(float cx, float cy, float radius,
								 const std::vector<GradientStop>& stops) {
	if (m_renderer) {
		m_renderer->setRadialGradient(cx, cy, radius, stops);
	}
}

void Graphics::setConicGradient(float cx, float cy, float angle,
								const std::vector<GradientStop>& stops) {
	if (m_renderer) {
		m_renderer->setConicGradient(cx, cy, angle, stops);
	}
}

void Graphics::clearGradient() {
	if (m_renderer) {
		m_renderer->clearGradient();
	}
}

void Graphics::clear(float r, float g, float b, float a) {
	if (m_renderer) {
		m_renderer->clear(glm::vec4(r, g, b, a));
	} else {
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}

void Graphics::save() {
	pushMatrix();
}

void Graphics::restore() {
	popMatrix();
}

void Graphics::setRenderer(IRenderer* renderer) {
	m_renderer = renderer;
}

void Graphics::setCanvasSize(int width, int height) {
	m_canvasWidth = width;
	m_canvasHeight = height;
}

} // namespace rigkit
