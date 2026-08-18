#define _USE_MATH_DEFINES
#include "rendering/OpenGLRenderer.h"

#include "rendering/U_gladGlfw.h"
#include "rendering/RendererRegistry.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
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
attribute vec3 aPos;
uniform mat4 model;
uniform mat4 projection;
void main() {
	gl_Position = projection * model * vec4(aPos, 1.0);
}
)";
	const char* fsSrc = R"(#version 100
precision mediump float;
uniform vec4 uColor;
void main() {
	gl_FragColor = uColor;
}
)";
#else
	const char* vsSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 projection;
void main() {
	gl_Position = projection * model * vec4(aPos, 1.0);
}
)";
	const char* fsSrc = R"(#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
void main() {
	FragColor = uColor;
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
	m_uColor = glGetUniformLocation(m_program, "uColor");

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2048, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
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
	// Origin top-left, y grows down — design pixels, not framebuffer pixels.
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

void OpenGLRenderer::endFrame() {}

void OpenGLRenderer::clear(const glm::vec4& color) {
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderer::drawArrays(unsigned mode, const std::vector<glm::vec2>& pts,
								const glm::vec4& color) {
	if (!m_initialized || pts.empty()) {
		return;
	}
	std::vector<float> verts;
	verts.reserve(pts.size() * 3);
	for (const auto& p : pts) {
		verts.push_back(p.x);
		verts.push_back(p.y);
		verts.push_back(0.f);
	}
	updateProjection();
	glUniform4fv(m_uColor, 1, glm::value_ptr(color));
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
					verts.data());
	glDrawArrays(static_cast<GLenum>(mode), 0, static_cast<GLsizei>(pts.size()));
	glBindVertexArray(0);
}

void OpenGLRenderer::drawFilledPoly(const std::vector<glm::vec2>& pts, const glm::vec4& color) {
	if (pts.size() < 3) {
		return;
	}
	drawArrays(GL_TRIANGLE_FAN, pts, color);
}

void OpenGLRenderer::drawStrokedPoly(const std::vector<glm::vec2>& pts, bool closed,
									 const glm::vec4& color, float width) {
	if (pts.size() < 2) {
		return;
	}
#if !defined(RIGKIT_GLES)
	glLineWidth(std::max(1.f, width));
#endif
	drawArrays(closed ? GL_LINE_LOOP : GL_LINE_STRIP, pts, color);
}

void OpenGLRenderer::drawOutline(const std::vector<glm::vec2>& pts, const Paint& paint) {
	if (paint.mode == Paint::Mode::Fill) {
		drawFilledPoly(pts, paint.color);
	} else {
		drawStrokedPoly(pts, true, paint.color, paint.strokeWidth);
	}
}

std::vector<glm::vec2> OpenGLRenderer::ellipsePoints(float cx, float cy, float rx, float ry,
													 int segments) const {
	std::vector<glm::vec2> pts;
	pts.reserve(static_cast<size_t>(segments));
	for (int i = 0; i < segments; ++i) {
		const float t = static_cast<float>(2.0 * M_PI * i / segments);
		pts.emplace_back(cx + rx * std::cos(t), cy + ry * std::sin(t));
	}
	return pts;
}

void OpenGLRenderer::drawLine(float x1, float y1, float x2, float y2, const Paint& paint) {
	drawStrokedPoly({{x1, y1}, {x2, y2}}, false, paint.color, paint.strokeWidth);
}

void OpenGLRenderer::drawRect(float x, float y, float width, float height, const Paint& paint) {
	drawOutline({{x, y}, {x + width, y}, {x + width, y + height}, {x, y + height}}, paint);
}

void OpenGLRenderer::drawCircle(float x, float y, float radius, const Paint& paint) {
	drawEllipse(x, y, radius * 2.f, radius * 2.f, paint);
}

void OpenGLRenderer::drawEllipse(float x, float y, float width, float height, const Paint& paint) {
	// Center + full width/height (same convention as Blend2DRenderer).
	drawOutline(ellipsePoints(x, y, width * 0.5f, height * 0.5f, 48), paint);
}

void OpenGLRenderer::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
								  const Paint& paint) {
	drawOutline({{x1, y1}, {x2, y2}, {x3, y3}}, paint);
}

void OpenGLRenderer::drawPolygon(const std::vector<glm::vec2>& points, const Paint& paint) {
	drawOutline(points, paint);
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
	drawFilledPoly(m_path, color);
}

void OpenGLRenderer::stroke(const glm::vec4& color, float width) {
	drawStrokedPoly(m_path, true, color, width);
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
	// Placeholder bar until a font pack lands — keeps Draw honest without silent no-op.
	drawRect(x, y, static_cast<float>(text.size()) * 8.f, 12.f, Paint::fill(color));
}

glm::vec2 OpenGLRenderer::getTextBounds(const std::string& text) {
	if (m_textBackend) {
		return m_textBackend->getTextBounds(text);
	}
	return {static_cast<float>(text.size()) * 8.f, 12.f};
}

void OpenGLRenderer::pushMatrix() {
	m_matrixStack.push_back(m_model);
}

void OpenGLRenderer::popMatrix() {
	if (!m_matrixStack.empty()) {
		m_model = m_matrixStack.back();
		m_matrixStack.pop_back();
	}
}

void OpenGLRenderer::translate(float x, float y) {
	m_model = glm::translate(m_model, glm::vec3(x, y, 0.f));
}

void OpenGLRenderer::rotate(float angle) {
	m_model = glm::rotate(m_model, angle, glm::vec3(0.f, 0.f, 1.f));
}

void OpenGLRenderer::scale(float sx, float sy) {
	m_model = glm::scale(m_model, glm::vec3(sx, sy, 1.f));
}

void OpenGLRenderer::resetMatrix() {
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
