#pragma once

#include <glm/glm.hpp>
#include <string>

namespace rigkit {

/**
 * @brief Optional filled-text present path for OpenGLRenderer.
 * @details Packs (e.g. rigVarFont) install a backend so setFont / drawText /
 * getTextBounds stop being the placeholder bar. Filled atlas quads only  - 
 * outline stroking is not part of this seam. Null backend keeps the bar.
 */
struct ITextBackend {
	virtual ~ITextBackend() = default;
	virtual void setFont(const std::string& fontPath, float size) = 0;
	virtual void drawText(const std::string& text, float x, float y, const glm::vec4& color) = 0;
	virtual glm::vec2 getTextBounds(const std::string& text) = 0;
};

} // namespace rigkit
