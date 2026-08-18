#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "rendering/IRenderer.h"
#include "rendering/ITextBackend.h"

namespace rigkit {

/**
 * @brief Default host Draw fulfillment — immediate GL primitives.
 * @details Draws to the current framebuffer (typically the window). Origin is
 * top-left, y grows down. Each primitive fills or strokes according to the
 * Paint it is given. Consecutive triangles merge into one draw regardless of
 * colour, and the batch is issued at the next topology change, transform, or
 * endFrame; call order still decides what paints on top. Does not create a
 * window or swap buffers. Gradients and saveTo* are no-ops until a richer pack
 * implements them.
 * @see IRenderer, Paint, MEcs::setPresentRenderer, Graphics
 */
class OpenGLRenderer : public IRenderer {
  public:
	OpenGLRenderer();
	~OpenGLRenderer() override;

	bool initialize(int width, int height) override;
	void shutdown() override;
	void resize(int width, int height) override;
	void setFramebufferSize(int width, int height) override;

	void beginFrame() override;
	void endFrame() override;
	void clear(const glm::vec4& color) override;

	void drawLine(float x1, float y1, float x2, float y2, const Paint& paint) override;
	void drawRect(float x, float y, float width, float height, const Paint& paint) override;
	void drawCircle(float x, float y, float radius, const Paint& paint) override;
	void drawEllipse(float x, float y, float width, float height, const Paint& paint) override;
	void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
					  const Paint& paint) override;
	void drawPolygon(const std::vector<glm::vec2>& points, const Paint& paint) override;
	void drawTriangles(const std::vector<glm::vec2>& pts, const Paint& paint) override;
	void drawLines(const std::vector<glm::vec2>& pts, const Paint& paint) override;

	void beginPath() override;
	void moveTo(float x, float y) override;
	void lineTo(float x, float y) override;
	void curveTo(float cx1, float cy1, float cx2, float cy2, float x, float y) override;
	void closePath() override;
	void fill(const glm::vec4& color) override;
	void stroke(const glm::vec4& color, float width) override;

	void setFont(const std::string& fontPath, float size) override;
	void drawText(const std::string& text, float x, float y, const glm::vec4& color) override;
	glm::vec2 getTextBounds(const std::string& text) override;

	/** @brief Install/clear optional filled-text backend (owned by the pack). */
	void setTextBackend(ITextBackend* backend) { m_textBackend = backend; }
	ITextBackend* textBackend() const { return m_textBackend; }

	void pushMatrix() override;
	void popMatrix() override;
	void translate(float x, float y) override;
	void rotate(float angle) override;
	void scale(float sx, float sy) override;
	void resetMatrix() override;

	void setLinearGradient(float x1, float y1, float x2, float y2,
						   const std::vector<GradientStop>& stops) override;
	void setRadialGradient(float cx, float cy, float radius,
						   const std::vector<GradientStop>& stops) override;
	void setConicGradient(float cx, float cy, float angle,
						  const std::vector<GradientStop>& stops) override;
	void clearGradient() override;

	bool saveToFile(const std::string& filename) override;
	bool saveToMemory(std::vector<uint8_t>& data) override;

	RendererType getType() const override { return RendererType::OpenGL; }
	std::string getName() const override { return "OpenGL"; }
	bool isInitialized() const override { return m_initialized; }
	int getWidth() const override { return m_width; }
	int getHeight() const override { return m_height; }
	uint8_t* getPixelBuffer() override { return nullptr; }

  private:
	// Position plus packed RGBA8. Colour travels per vertex so shapes of
	// different colours still share one draw call.
	struct Vertex {
		glm::vec2 pos;
		uint32_t color = 0;
	};

	void updateProjection();
	// Keep the open batch when topology and line width match, else flush and
	// start a new one. Vertices append after this, so call order stays paint
	// order. Fills pass 0 for lineWidth — it has no meaning for triangles.
	void openBatch(unsigned mode, float lineWidth);
	void enqueue(unsigned mode, std::span<const glm::vec2> pts, uint32_t color, float lineWidth);
	void flush();
	void drawFilledPoly(std::span<const glm::vec2> pts, uint32_t color);
	void drawStrokedPoly(std::span<const glm::vec2> pts, bool closed, uint32_t color, float width);
	/** @brief Fill or stroke a closed outline according to @p paint. */
	void drawOutline(std::span<const glm::vec2> pts, const Paint& paint);
	// Fills and returns the reusable scratch ring, so circles cost no
	// allocation per frame. Valid until the next call.
	const std::vector<glm::vec2>& ellipsePoints(float cx, float cy, float rx, float ry);

	bool m_initialized = false;
	int m_width = 0;
	int m_height = 0;
	int m_fbWidth = 0;
	int m_fbHeight = 0;

	glm::mat4 m_model{1.f};
	std::vector<glm::mat4> m_matrixStack;

	std::vector<glm::vec2> m_path;
	bool m_pathOpen = false;

	unsigned m_program = 0;
	unsigned m_vao = 0;
	unsigned m_vbo = 0;
	int m_uModel = -1;
	int m_uProjection = -1;
	size_t m_vboBytes = 0;

	std::vector<Vertex> m_batch;
	unsigned m_batchMode = 0;
	float m_batchLineWidth = 0.f;
	std::vector<glm::vec2> m_ellipseScratch;

	ITextBackend* m_textBackend = nullptr;
};

} // namespace rigkit
