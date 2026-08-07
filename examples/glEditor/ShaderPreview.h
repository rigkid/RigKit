#pragma once

#include <glm/glm.hpp>
#include <string>

/**
 * @brief Example-local Shadertoy-style GLES preview (not a pack).
 * @details Wraps user `mainImage` into a host fragment shader, compiles with
 * last-good program swap on failure, draws a fullscreen quad into an FBO.
 */
class ShaderPreview {
  public:
	ShaderPreview() = default;
	~ShaderPreview();

	ShaderPreview(const ShaderPreview&) = delete;
	ShaderPreview& operator=(const ShaderPreview&) = delete;

	/** @brief Compile user Shadertoy source. Keeps prior program on failure. */
	bool compile(const std::string& userSource);

	/** @brief Render into the FBO at the given size (resized as needed). */
	void render(int width, int height, float iTime, const glm::vec4& iMouse);

	unsigned int colorTexture() const { return m_colorTex; }
	int texWidth() const { return m_width; }
	int texHeight() const { return m_height; }
	bool hasProgram() const { return m_program != 0; }
	const std::string& error() const { return m_error; }
	bool hasError() const { return !m_error.empty(); }

	void setPaused(bool paused) { m_paused = paused; }
	bool paused() const { return m_paused; }
	void setSpeed(float speed) { m_speed = speed > 0.f ? speed : 1.f; }
	float speed() const { return m_speed; }
	void resetTime() { m_time = 0.f; }
	float time() const { return m_time; }
	void advanceTime(float dt) {
		if (!m_paused) {
			m_time += dt * m_speed;
		}
		m_fpsAccum += dt;
		m_fpsFrames++;
		if (m_fpsAccum >= 0.25f) {
			m_fps = static_cast<float>(m_fpsFrames) / m_fpsAccum;
			m_fpsAccum = 0.f;
			m_fpsFrames = 0;
		}
	}

	float fps() const { return m_fps; }

  private:
	void ensureFbo(int width, int height);
	void destroyFbo();
	void destroyProgram(unsigned int& program);
	static std::string wrapFragment(const std::string& userSource);
	static unsigned int compileShader(unsigned int type, const char* src, std::string& log);
	static unsigned int linkProgram(unsigned int vs, unsigned int fs, std::string& log);

	unsigned int m_fbo = 0;
	unsigned int m_colorTex = 0;
	unsigned int m_program = 0;
	unsigned int m_vbo = 0;
	unsigned int m_vao = 0;
	int m_width = 0;
	int m_height = 0;
	int m_locResolution = -1;
	int m_locTime = -1;
	int m_locMouse = -1;
	std::string m_error;
	bool m_paused = false;
	float m_speed = 1.f;
	float m_time = 0.f;
	float m_fps = 0.f;
	float m_fpsAccum = 0.f;
	int m_fpsFrames = 0;
};
