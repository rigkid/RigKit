#include "core/canvas/Canvas.h"

#include "rendering/U_gladGlfw.h"

#include <filesystem>
#include <spdlog/spdlog.h>

#include "core/ISettings.h"
#include "core/json.h"
#include "rendering/Graphics.h"
#include "rendering/IRenderer.h"

namespace rigkit {

struct Canvas::Impl {
	GLuint framebuffer = 0;
	GLuint colorTexture = 0;
	GLuint depthRenderbuffer = 0;
	GLuint shaderProgram = 0;
	GLuint VAO = 0;
	GLuint VBO = 0;
};

Canvas::Canvas(const CanvasSettings& settings, RigKitEngine* engine)
	: m_width(settings.width), m_height(settings.height), m_settings(settings),
	  m_impl(std::make_unique<Impl>()), m_graphics(std::make_shared<Graphics>()), m_engine(engine) {
	m_graphics->setCanvasSize(m_width, m_height);
	initFramebuffer();
	initShaders();
	clear(1.0f, 1.0f, 1.0f, 1.0f);
}

Canvas::~Canvas() {
	if (m_impl->framebuffer) {
		glDeleteFramebuffers(1, &m_impl->framebuffer);
	}
	if (m_impl->colorTexture) {
		glDeleteTextures(1, &m_impl->colorTexture);
	}
	if (m_impl->depthRenderbuffer) {
		glDeleteRenderbuffers(1, &m_impl->depthRenderbuffer);
	}
	if (m_impl->shaderProgram) {
		glDeleteProgram(m_impl->shaderProgram);
	}
	if (m_impl->VAO) {
		glDeleteVertexArrays(1, &m_impl->VAO);
	}
	if (m_impl->VBO) {
		glDeleteBuffers(1, &m_impl->VBO);
	}
}

void Canvas::resize(int width, int height) {
	m_width = width;
	m_height = height;
	m_graphics->setCanvasSize(m_width, m_height);
	initFramebuffer();
	// Set default background to white after resize
	clear(1.0f, 1.0f, 1.0f, 1.0f);
}

void Canvas::clear() {
	clear(1.0f, 1.0f, 1.0f, 1.0f);
}

void Canvas::clear(float r, float g, float b, float a) {
	glBindFramebuffer(GL_FRAMEBUFFER, m_impl->framebuffer);
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Canvas::background(float r, float g, float b, float a) {
	clear(r, g, b, a);
}

void Canvas::fill(float r, float g, float b, float a) {
	m_graphics->setFillColor(r, g, b, a);
}

void Canvas::noFill() {
	m_graphics->noFill();
}

void Canvas::stroke(float r, float g, float b, float a) {
	m_graphics->setStrokeColor(r, g, b, a);
}

void Canvas::noStroke() {
	m_graphics->noStroke();
}

void Canvas::strokeWeight(float weight) {
	m_graphics->setStrokeWidth(weight);
}

void Canvas::rect(float x, float y, float width, float height) {
	m_graphics->drawRect(x, y, width, height);
}

void Canvas::ellipse(float x, float y, float width, float height) {
	m_graphics->drawEllipse(x, y, width, height);
}

void Canvas::line(float x1, float y1, float x2, float y2) {
	m_graphics->drawLine(x1, y1, x2, y2);
}

void Canvas::triangle(float x1, float y1, float x2, float y2, float x3, float y3) {
	m_graphics->drawTriangle(x1, y1, x2, y2, x3, y3);
}

void Canvas::circle(float x, float y, float diameter) {
	ellipse(x, y, diameter, diameter);
}

void Canvas::text(const std::string& text, float x, float y) {
	if (m_graphics) {
		m_graphics->setFont("Arial", m_textSize);
		m_graphics->drawText(text, x, y);
	}
}

void Canvas::textSize(float size) {
	m_textSize = size;
}

void Canvas::textAlign(int align) {
	m_textAlign = align;
}

void Canvas::pushMatrix() {
	if (m_graphics) {
		m_graphics->pushMatrix();
	}
}

void Canvas::popMatrix() {
	if (m_graphics) {
		m_graphics->popMatrix();
	}
}

void Canvas::translate(float x, float y) {
	if (m_graphics) {
		m_graphics->translate(x, y);
	}
}

void Canvas::rotate(float angle) {
	if (m_graphics) {
		m_graphics->rotate(angle);
	}
}

void Canvas::scale(float x, float y) {
	if (m_graphics) {
		m_graphics->scale(x, y);
	}
}

void Canvas::beginOffscreen() {
	if (m_impl && m_impl->framebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, m_impl->framebuffer);
		glViewport(0, 0, m_width, m_height);
	}
}

void Canvas::endOffscreen() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int Canvas::getColorTexture() const {
	return m_impl->colorTexture;
}

void Canvas::initFramebuffer() {
	// Create framebuffer for off-screen rendering
	glGenFramebuffers(1, &m_impl->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_impl->framebuffer);

	// Create color texture
	glGenTextures(1, &m_impl->colorTexture);
	glBindTexture(GL_TEXTURE_2D, m_impl->colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   m_impl->colorTexture, 0);

	// Create depth renderbuffer
	glGenRenderbuffers(1, &m_impl->depthRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_impl->depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
							  m_impl->depthRenderbuffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		// Handle framebuffer error
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Canvas::initShaders() {
#if defined(RIGKIT_GLES)
	// GLES2 / ANGLE / Pi — GLSL ES 1.00 (no layout qualifiers).
	const char* vertexShaderSource = R"(
		#version 100
		attribute vec3 aPos;
		attribute vec2 aTexCoord;
		varying vec2 TexCoord;
		uniform mat4 model;
		uniform mat4 projection;
		void main() {
			gl_Position = projection * model * vec4(aPos, 1.0);
			TexCoord = aTexCoord;
		}
	)";
	const char* fragmentShaderSource = R"(
		#version 100
		precision mediump float;
		varying vec2 TexCoord;
		uniform sampler2D texture1;
		void main() {
			gl_FragColor = texture2D(texture1, TexCoord);
		}
	)";
#else
	const char* vertexShaderSource = R"(
		#version 330 core
		layout (location = 0) in vec3 aPos;
		layout (location = 1) in vec2 aTexCoord;
		out vec2 TexCoord;
		uniform mat4 model;
		uniform mat4 projection;
		void main() {
			gl_Position = projection * model * vec4(aPos, 1.0);
			TexCoord = aTexCoord;
		}
	)";
	const char* fragmentShaderSource = R"(
		#version 330 core
		out vec4 FragColor;
		in vec2 TexCoord;
		uniform sampler2D texture1;
		void main() {
			FragColor = texture(texture1, TexCoord);
		}
	)";
#endif

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	m_impl->shaderProgram = glCreateProgram();
	glAttachShader(m_impl->shaderProgram, vertexShader);
	glAttachShader(m_impl->shaderProgram, fragmentShader);
#if defined(RIGKIT_GLES)
	glBindAttribLocation(m_impl->shaderProgram, 0, "aPos");
	glBindAttribLocation(m_impl->shaderProgram, 1, "aTexCoord");
#endif
	glLinkProgram(m_impl->shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Set up vertex data
	glGenVertexArrays(1, &m_impl->VAO);
	glGenBuffers(1, &m_impl->VBO);

	glBindVertexArray(m_impl->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_impl->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 5, NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);
}

void Canvas::switchRenderer(RendererType type) {
	auto renderer = createRenderer(type);
	if (!renderer) {
		spdlog::error("switchRenderer: no factory for renderer type {}", static_cast<int>(type));
		return;
	}
	setRenderer(std::move(renderer));
}

void Canvas::setRenderer(std::shared_ptr<IRenderer> renderer) {
	if (!renderer) {
		return;
	}
	if (!renderer->initialize(m_width, m_height)) {
		spdlog::error("Failed to initialize renderer: {}", renderer->getName());
		return;
	}
	m_ownedRenderer = std::move(renderer);
	m_graphics->setRenderer(m_ownedRenderer.get());
	spdlog::info("Set canvas renderer to: {}", m_ownedRenderer->getName());
}

RendererType Canvas::getRendererType() const {
	if (m_graphics && m_graphics->getRenderer()) {
		return m_graphics->getRenderer()->getType();
	}
	return RendererType::OpenGL; // Host default
}

json Canvas::getSettings() const {
	json j;
	j["width"] = m_width;
	j["height"] = m_height;
	j["name"] = m_name;
	j["background"] = {m_settings.r, m_settings.g, m_settings.b, m_settings.a};
	j["samples"] = m_settings.samples;
	return j;
}

void Canvas::setSettings(const json& settings) {
	if (settings.contains("width"))
		m_width = settings["width"].get<int>();
	if (settings.contains("height"))
		m_height = settings["height"].get<int>();
	if (settings.contains("name"))
		m_name = settings["name"].get<std::string>();
	if (settings.contains("background")) {
		auto bg = settings["background"];
		if (bg.is_array() && bg.size() == 4) {
			m_settings.r = bg[0].get<float>();
			m_settings.g = bg[1].get<float>();
			m_settings.b = bg[2].get<float>();
			m_settings.a = bg[3].get<float>();
		}
	}
	if (settings.contains("samples"))
		m_settings.samples = settings["samples"].get<int>();
	resize(m_width, m_height);
}

} // namespace rigkit
