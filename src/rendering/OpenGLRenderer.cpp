#define _USE_MATH_DEFINES
#include "rendering/OpenGLRenderer.h"

#include "rendering/U_gladGlfw.h"
#include "rendering/RendererRegistry.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <spdlog/spdlog.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace rigkit {
namespace {

struct OpenGLRendererRegistrar {
	OpenGLRendererRegistrar() {
		RendererRegistry::instance().registerFactory(RendererType::OpenGL, []() {
			return std::shared_ptr<IRenderer>(std::make_shared<OpenGLRenderer>());
		});
	}
};
static OpenGLRendererRegistrar g_openglRendererRegistrar;

// Byte order matches a normalized GL_UNSIGNED_BYTE RGBA attribute.
uint32_t packColor(const glm::vec4& c) {
	auto channel = [](float v) {
		return static_cast<uint32_t>(std::clamp(v, 0.f, 1.f) * 255.f + 0.5f);
	};
	return channel(c.r) | (channel(c.g) << 8) | (channel(c.b) << 16) | (channel(c.a) << 24);
}

// Segment count that keeps the chord within kTolerance of the true arc, so
// small circles stay cheap and large ones stay round.
int arcSegments(float radius) {
	constexpr float kTolerance = 0.3f; // design pixels
	if (radius <= kTolerance) {
		return 8;
	}
	const float step = 2.f * std::acos(std::clamp(1.f - kTolerance / radius, -1.f, 1.f));
	if (step <= 0.f) {
		return 256;
	}
	return std::clamp(static_cast<int>(std::ceil(2.0 * M_PI / step)), 8, 256);
}

} // namespace

OpenGLRenderer::OpenGLRenderer() = default;

OpenGLRenderer::~OpenGLRenderer() {
	shutdown();
}

bool OpenGLRenderer::initialize(int width, int height) {
	if (m_initialized) {
		resize(width, height);
		return true;
	}

#if defined(RIGKIT_GLES)
	const char* vsSrc = R"(#version 100
attribute vec2 aPos;
attribute vec4 aColor;
uniform mat4 model;
uniform mat4 projection;
varying vec4 vColor;
void main() {
	vColor = aColor;
	gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
)";
	const char* fsSrc = R"(#version 100
precision mediump float;
varying vec4 vColor;
void main() {
	gl_FragColor = vColor;
}
)";
#else
	const char* vsSrc = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
uniform mat4 model;
uniform mat4 projection;
out vec4 vColor;
void main() {
	vColor = aColor;
	gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
)";
	const char* fsSrc = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
	FragColor = vColor;
}
)";
#endif

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vsSrc, nullptr);
	glCompileShader(vs);
	GLint ok = 0;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(vs, 512, nullptr, log);
		spdlog::error("[OpenGLRenderer] vertex shader: {}", log);
		glDeleteShader(vs);
		return false;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fsSrc, nullptr);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(fs, 512, nullptr, log);
		spdlog::error("[OpenGLRenderer] fragment shader: {}", log);
		glDeleteShader(vs);
		glDeleteShader(fs);
		return false;
	}

	m_program = glCreateProgram();
	glAttachShader(m_program, vs);
	glAttachShader(m_program, fs);
#if defined(RIGKIT_GLES)
	glBindAttribLocation(m_program, 0, "aPos");
	glBindAttribLocation(m_program, 1, "aColor");
#endif
	glLinkProgram(m_program);
	glDeleteShader(vs);
	glDeleteShader(fs);
	glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetProgramInfoLog(m_program, 512, nullptr, log);
		spdlog::error("[OpenGLRenderer] program link: {}", log);
		glDeleteProgram(m_program);
		m_program = 0;
		return false;
	}

	m_uModel = glGetUniformLocation(m_program, "model");
	m_uProjection = glGetUniformLocation(m_program, "projection");

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	m_vboBytes = 2048 * sizeof(Vertex);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vboBytes), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
						  reinterpret_cast<void*>(offsetof(Vertex, pos)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex),
						  reinterpret_cast<void*>(offsetof(Vertex, color)));
	glBindVertexArray(0);

	m_model = glm::mat4(1.f);
	m_initialized = true;
	resize(width, height);
	spdlog::info("[OpenGLRenderer] initialized {}x{}", width, height);
	return true;
}

void OpenGLRenderer::shutdown() {
	if (!m_initialized) {
		return;
	}
	flush();
	if (m_vbo) {
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
	}
	if (m_vao) {
		glDeleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}
	if (m_program) {
		glDeleteProgram(m_program);
		m_program = 0;
	}
	m_initialized = false;
	m_vboBytes = 0;
}

void OpenGLRenderer::resize(int width, int height) {
	m_width = width > 0 ? width : 1;
	m_height = height > 0 ? height : 1;
	if (m_fbWidth <= 0 || m_fbHeight <= 0) {
		m_fbWidth = m_width;
		m_fbHeight = m_height;
	}
}

void OpenGLRenderer::setFramebufferSize(int width, int height) {
	m_fbWidth = width > 0 ? width : 1;
	m_fbHeight = height > 0 ? height : 1;
}

void OpenGLRenderer::updateProjection() {
	// Origin top-left, y grows down - design pixels, not framebuffer pixels.
	const glm::mat4 projection =
		glm::ortho(0.f, static_cast<float>(m_width), static_cast<float>(m_height), 0.f, -1.f, 1.f);
	glUseProgram(m_program);
	glUniformMatrix4fv(m_uProjection, 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(m_uModel, 1, GL_FALSE, glm::value_ptr(m_model));
}

void OpenGLRenderer::beginFrame() {
	if (!m_initialized) {
		return;
	}
	flush();
	const int vpW = m_fbWidth > 0 ? m_fbWidth : m_width;
	const int vpH = m_fbHeight > 0 ? m_fbHeight : m_height;
	glViewport(0, 0, vpW, vpH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
#if !defined(RIGKIT_GLES)
	glEnable(GL_MULTISAMPLE);
#endif
	updateProjection();
}

void OpenGLRenderer::endFrame() {
	flush();
}

void OpenGLRenderer::clear(const glm::vec4& color) {
	flush();
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderer::openBatch(unsigned mode, float lineWidth) {
	if (!m_batch.empty() && m_batchMode == mode && m_batchLineWidth == lineWidth) {
		return;
	}
	flush();
	m_batchMode = mode;
	m_batchLineWidth = lineWidth;
}

void OpenGLRenderer::enqueue(unsigned mode, std::span<const glm::vec2> pts, uint32_t color,
							 float lineWidth) {
	if (!m_initialized || pts.empty()) {
		return;
	}
	openBatch(mode, lineWidth);
	m_batch.reserve(m_batch.size() + pts.size());
	for (const glm::vec2& p : pts) {
		m_batch.push_back({p, color});
	}
}

void OpenGLRenderer::flush() {
	if (!m_initialized || m_batch.empty()) {
		return;
	}
	updateProjection();
#if !defined(RIGKIT_GLES)
	if (m_batchMode == GL_LINES) {
		glLineWidth(std::max(1.f, m_batchLineWidth));
	}
#endif
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	const size_t bytes = m_batch.size() * sizeof(Vertex);
	if (bytes > m_vboBytes) {
		m_vboBytes = bytes;
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vboBytes), nullptr,
					 GL_DYNAMIC_DRAW);
	}
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bytes), m_batch.data());
	glDrawArrays(static_cast<GLenum>(m_batchMode), 0, static_cast<GLsizei>(m_batch.size()));
	glBindVertexArray(0);
	m_batch.clear();
}

void OpenGLRenderer::drawFilledPoly(std::span<const glm::vec2> pts, uint32_t color) {
	if (!m_initialized || pts.size() < 3) {
		return;
	}
	// Convex fan as independent triangles, so consecutive fills merge into one
	// batch. Concave outlines and holes need a real tessellator (Blend2D pack).
	openBatch(GL_TRIANGLES, 0.f);
	reserveBatch((pts.size() - 2) * 3);
	for (size_t i = 1; i + 1 < pts.size(); ++i) {
		m_batch.push_back({pts[0], color});
		m_batch.push_back({pts[i], color});
		m_batch.push_back({pts[i + 1], color});
	}
}

void OpenGLRenderer::reserveBatch(size_t add) {
	const size_t need = m_batch.size() + add;
	if (need > m_batch.capacity()) {
		m_batch.reserve(std::max(need, m_batch.capacity() * 2));
	}
}

void OpenGLRenderer::drawStrokedPoly(std::span<const glm::vec2> pts, bool closed, uint32_t color,
									 float width) {
	if (!m_initialized || pts.size() < 2) {
		return;
	}
	// Strip or loop as independent segments, so consecutive strokes of the same
	// width merge. Two points are already the whole loop - wrapping redraws it.
	const bool wrap = closed && pts.size() > 2;
	openBatch(GL_LINES, width);
	reserveBatch((pts.size() - 1 + (wrap ? 1u : 0u)) * 2);
	for (size_t i = 0; i + 1 < pts.size(); ++i) {
		m_batch.push_back({pts[i], color});
		m_batch.push_back({pts[i + 1], color});
	}
	if (wrap) {
		m_batch.push_back({pts.back(), color});
		m_batch.push_back({pts.front(), color});
	}
}

void OpenGLRenderer::drawOutline(std::span<const glm::vec2> pts, const Paint& paint) {
	if (paint.mode == Paint::Mode::Fill) {
		drawFilledPoly(pts, packColor(paint.color));
	} else {
		drawStrokedPoly(pts, true, packColor(paint.color), paint.strokeWidth);
	}
}

const std::vector<glm::vec2>& OpenGLRenderer::ellipsePoints(float cx, float cy, float rx,
															float ry) {
	const int segments = arcSegments(std::max(std::abs(rx), std::abs(ry)));
	m_ellipseScratch.clear();
	m_ellipseScratch.reserve(static_cast<size_t>(segments));
	for (int i = 0; i < segments; ++i) {
		const float t = static_cast<float>(2.0 * M_PI * i / segments);
		m_ellipseScratch.emplace_back(cx + rx * std::cos(t), cy + ry * std::sin(t));
	}
	return m_ellipseScratch;
}

void OpenGLRenderer::drawLine(float x1, float y1, float x2, float y2, const Paint& paint) {
	const std::array<glm::vec2, 2> pts{{{x1, y1}, {x2, y2}}};
	drawStrokedPoly(pts, false, packColor(paint.color), paint.strokeWidth);
}

void OpenGLRenderer::drawRect(float x, float y, float width, float height, const Paint& paint) {
	const std::array<glm::vec2, 4> pts{
		{{x, y}, {x + width, y}, {x + width, y + height}, {x, y + height}}};
	drawOutline(pts, paint);
}

void OpenGLRenderer::drawCircle(float x, float y, float radius, const Paint& paint) {
	drawEllipse(x, y, radius * 2.f, radius * 2.f, paint);
}

void OpenGLRenderer::drawEllipse(float x, float y, float width, float height, const Paint& paint) {
	// Center + full width/height (same convention as Blend2DRenderer).
	drawOutline(ellipsePoints(x, y, width * 0.5f, height * 0.5f), paint);
}

void OpenGLRenderer::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
								  const Paint& paint) {
	const std::array<glm::vec2, 3> pts{{{x1, y1}, {x2, y2}, {x3, y3}}};
	drawOutline(pts, paint);
}

void OpenGLRenderer::drawPolygon(const std::vector<glm::vec2>& points, const Paint& paint) {
	drawOutline(points, paint);
}

void OpenGLRenderer::drawTriangles(const std::vector<glm::vec2>& pts, const Paint& paint) {
	if (pts.size() < 3) {
		return;
	}
	enqueue(GL_TRIANGLES, pts, packColor(paint.color), 0.f);
}

void OpenGLRenderer::drawLines(const std::vector<glm::vec2>& pts, const Paint& paint) {
	if (pts.size() < 2) {
		return;
	}
	enqueue(GL_LINES, pts, packColor(paint.color), paint.strokeWidth);
}

void OpenGLRenderer::beginPath() {
	m_path.clear();
	m_pathOpen = true;
}

void OpenGLRenderer::moveTo(float x, float y) {
	if (m_pathOpen) {
		m_path.emplace_back(x, y);
	}
}

void OpenGLRenderer::lineTo(float x, float y) {
	if (m_pathOpen) {
		m_path.emplace_back(x, y);
	}
}

void OpenGLRenderer::curveTo(float, float, float, float, float x, float y) {
	lineTo(x, y);
}

void OpenGLRenderer::closePath() {
	m_pathOpen = false;
}

void OpenGLRenderer::fill(const glm::vec4& color) {
	drawFilledPoly(m_path, packColor(color));
}

void OpenGLRenderer::stroke(const glm::vec4& color, float width) {
	drawStrokedPoly(m_path, true, packColor(color), width);
}

void OpenGLRenderer::setFont(const std::string& fontPath, float size) {
	if (m_textBackend) {
		m_textBackend->setFont(fontPath, size);
		return;
	}
}

void OpenGLRenderer::drawText(const std::string& text, float x, float y, const glm::vec4& color) {
	if (m_textBackend) {
		m_textBackend->drawText(text, x, y, color);
		return;
	}
	// Placeholder bar until a font pack lands, so text occupies space instead
	// of vanishing.
	drawRect(x, y, static_cast<float>(text.size()) * 8.f, 12.f, Paint::fill(color));
}

glm::vec2 OpenGLRenderer::getTextBounds(const std::string& text) {
	if (m_textBackend) {
		return m_textBackend->getTextBounds(text);
	}
	return {static_cast<float>(text.size()) * 8.f, 12.f};
}

void OpenGLRenderer::pushMatrix() {
	flush();
	m_matrixStack.push_back(m_model);
}

void OpenGLRenderer::popMatrix() {
	flush();
	if (!m_matrixStack.empty()) {
		m_model = m_matrixStack.back();
		m_matrixStack.pop_back();
	}
}

void OpenGLRenderer::translate(float x, float y) {
	flush();
	m_model = glm::translate(m_model, glm::vec3(x, y, 0.f));
}

void OpenGLRenderer::rotate(float angle) {
	flush();
	m_model = glm::rotate(m_model, angle, glm::vec3(0.f, 0.f, 1.f));
}

void OpenGLRenderer::scale(float sx, float sy) {
	flush();
	m_model = glm::scale(m_model, glm::vec3(sx, sy, 1.f));
}

void OpenGLRenderer::resetMatrix() {
	flush();
	m_model = glm::mat4(1.f);
	m_matrixStack.clear();
}

void OpenGLRenderer::setLinearGradient(float, float, float, float,
									   const std::vector<GradientStop>&) {}
void OpenGLRenderer::setRadialGradient(float, float, float, const std::vector<GradientStop>&) {}
void OpenGLRenderer::setConicGradient(float, float, float, const std::vector<GradientStop>&) {}
void OpenGLRenderer::clearGradient() {}

bool OpenGLRenderer::saveToFile(const std::string&) {
	return false;
}

bool OpenGLRenderer::saveToMemory(std::vector<uint8_t>&) {
	return false;
}

} // namespace rigkit
