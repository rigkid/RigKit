#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace rigkit {
class Graphics;
}

namespace rig {

/**
 * @brief Immediate-mode draw helpers.
 * @details Call setGraphics with a Graphics that already has an IRenderer.
 * Prefer entity creators (`rig/create.h`) when data should be inspectable.
 * @see Graphics, OpenGLRenderer
 */

void setGraphics(rigkit::Graphics* g);
rigkit::Graphics* graphics();

void setFill(float r, float g, float b, float a = 1.0f);
void setStroke(float r, float g, float b, float a = 1.0f);
void setStrokeWidth(float width);

void rect(float x, float y, float w, float h);
void ellipse(float x, float y, float w, float h);
void circle(float x, float y, float radius);
void line(float x1, float y1, float x2, float y2);
void triangle(float x1, float y1, float x2, float y2, float x3, float y3);
void polygon(const std::vector<glm::vec2>& points);

/**
 * @brief Draw a triangle list (x,y from vec3; z ignored for now).
 * @param indices Empty = sequential groups of three.
 */
void mesh(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices = {});

} // namespace rig
