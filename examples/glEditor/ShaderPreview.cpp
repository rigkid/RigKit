#include "ShaderPreview.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <vector>
#include "rendering/U_gladGlfw.h"

namespace {

constexpr const char* kVsGles = R"(#version 100
attribute vec2 aPos;
void main() {
	gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

constexpr const char* kVsGl = R"(#version 330 core
layout (location = 0) in vec2 aPos;
void main() {
	gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

} // namespace

ShaderPreview::~ShaderPreview() {
	destroyFbo();
	destroyProgram(m_program);
	if (m_vbo) {
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
	}
	if (m_vao) {
		glDeleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}
}

std::string ShaderPreview::wrapFragment(const std::string& userSource) {
#if defined(RIGKIT_GLES)
	std::string src;
	src.reserve(userSource.size() + 512);
	src += R"(#version 100
precision mediump float;
uniform vec3 iResolution;
uniform float iTime;
uniform vec4 iMouse;
)";
	src += userSource;
	src += R"(
void main() {
	vec4 fragColor = vec4(0.0);
	mainImage(fragColor, gl_FragCoord.xy);
	gl_FragColor = fragColor;
}
)";
	return src;
#else
	std::string src;
	src.reserve(userSource.size() + 512);
	src += R"(#version 330 core
uniform vec3 iResolution;
uniform float iTime;
uniform vec4 iMouse;
out vec4 FragColor;
)";
	src += userSource;
	src += R"(
void main() {
	vec4 fragColor = vec4(0.0);
	mainImage(fragColor, gl_FragCoord.xy);
	FragColor = fragColor;
}
)";
	return src;
#endif
}

unsigned int ShaderPreview::compileShader(unsigned int type, const char* src, std::string& log) {
	const GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	GLint ok = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		GLint len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		std::vector<char> buf(static_cast<size_t>(std::max(len, 1)));
		glGetShaderInfoLog(shader, len, nullptr, buf.data());
		log = buf.data();
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

unsigned int ShaderPreview::linkProgram(unsigned int vs, unsigned int fs, std::string& log) {
	const GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
#if defined(RIGKIT_GLES)
	glBindAttribLocation(program, 0, "aPos");
#endif
	glLinkProgram(program);
	GLint ok = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		GLint len = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		std::vector<char> buf(static_cast<size_t>(std::max(len, 1)));
		glGetProgramInfoLog(program, len, nullptr, buf.data());
		log = buf.data();
		glDeleteProgram(program);
		return 0;
	}
	return program;
}

void ShaderPreview::destroyProgram(unsigned int& program) {
	if (program) {
		glDeleteProgram(program);
		program = 0;
	}
}

void ShaderPreview::destroyFbo() {
	if (m_colorTex) {
		glDeleteTextures(1, &m_colorTex);
		m_colorTex = 0;
	}
	if (m_fbo) {
		glDeleteFramebuffers(1, &m_fbo);
		m_fbo = 0;
	}
	m_width = 0;
	m_height = 0;
}

void ShaderPreview::ensureFbo(int width, int height) {
	width = std::max(width, 1);
	height = std::max(height, 1);
	if (m_fbo && m_width == width && m_height == height) {
		return;
	}
	destroyFbo();
	m_width = width;
	m_height = height;

	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	glGenTextures(1, &m_colorTex);
	glBindTexture(GL_TEXTURE_2D, m_colorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				 nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		spdlog::error("[ShaderPreview] incomplete FBO");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool ShaderPreview::compile(const std::string& userSource) {
	std::string log;
#if defined(RIGKIT_GLES)
	const char* vsSrc = kVsGles;
#else
	const char* vsSrc = kVsGl;
#endif
	const GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc, log);
	if (!vs) {
		m_error = "vertex shader:\n" + log;
		return false;
	}

	const std::string fsSrc = wrapFragment(userSource);
	const GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc.c_str(), log);
	if (!fs) {
		glDeleteShader(vs);
		m_error = "fragment shader:\n" + log;
		return false;
	}

	const GLuint program = linkProgram(vs, fs, log);
	glDeleteShader(vs);
	glDeleteShader(fs);
	if (!program) {
		m_error = "link:\n" + log;
		return false;
	}

	// Success — swap; keep last-good until this point.
	destroyProgram(m_program);
	m_program = program;
	m_locResolution = glGetUniformLocation(m_program, "iResolution");
	m_locTime = glGetUniformLocation(m_program, "iTime");
	m_locMouse = glGetUniformLocation(m_program, "iMouse");
	m_error.clear();

	if (!m_vbo) {
		static const float kQuad[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(kQuad), kQuad, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
		glBindVertexArray(0);
	}
	return true;
}

void ShaderPreview::render(int width, int height, float /*iTimeUnused*/, const glm::vec4& iMouse) {
	ensureFbo(width, height);

	GLint prevFbo = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
	GLint prevViewport[4] = {};
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glViewport(0, 0, m_width, m_height);
	glClearColor(0.08f, 0.08f, 0.1f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	if (m_program && m_vao) {
		glUseProgram(m_program);
		if (m_locResolution >= 0) {
			glUniform3f(m_locResolution, static_cast<float>(m_width), static_cast<float>(m_height),
						1.f);
		}
		if (m_locTime >= 0) {
			glUniform1f(m_locTime, m_time);
		}
		if (m_locMouse >= 0) {
			glUniform4f(m_locMouse, iMouse.x, iMouse.y, iMouse.z, iMouse.w);
		}
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glUseProgram(0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}
