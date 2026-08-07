#include "author/rigDraw.h"
#include "rendering/Graphics.h"

namespace rig {
namespace {

rigkit::Graphics* g_gfx = nullptr;

} // namespace

void setGraphics(rigkit::Graphics* g) {
	g_gfx = g;
}

rigkit::Graphics* graphics() {
	return g_gfx;
}

void setFill(float r, float g, float b, float a) {
	if (g_gfx) {
		g_gfx->setFillColor(r, g, b, a);
	}
}

void setStroke(float r, float g, float b, float a) {
	if (g_gfx) {
		g_gfx->setStrokeColor(r, g, b, a);
	}
}

void setStrokeWidth(float width) {
	if (g_gfx) {
		g_gfx->setStrokeWidth(width);
	}
}

void rect(float x, float y, float w, float h) {
	if (g_gfx) {
		g_gfx->drawRect(x, y, w, h);
	}
}

void ellipse(float x, float y, float w, float h) {
	if (g_gfx) {
		g_gfx->drawEllipse(x, y, w, h);
	}
}

void circle(float x, float y, float radius) {
	if (g_gfx) {
		g_gfx->drawCircle(x, y, radius);
	}
}

void line(float x1, float y1, float x2, float y2) {
	if (g_gfx) {
		g_gfx->drawLine(x1, y1, x2, y2);
	}
}

void triangle(float x1, float y1, float x2, float y2, float x3, float y3) {
	if (g_gfx) {
		g_gfx->drawTriangle(x1, y1, x2, y2, x3, y3);
	}
}

void polygon(const std::vector<glm::vec2>& points) {
	if (g_gfx) {
		g_gfx->drawPolygon(points);
	}
}

void mesh(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices) {
	if (!g_gfx || positions.empty()) {
		return;
	}
	auto tri = [&](uint32_t i0, uint32_t i1, uint32_t i2) {
		const auto& a = positions[i0];
		const auto& b = positions[i1];
		const auto& c = positions[i2];
		g_gfx->drawTriangle(a.x, a.y, b.x, b.y, c.x, c.y);
	};
	if (indices.empty()) {
		for (size_t i = 0; i + 2 < positions.size(); i += 3) {
			tri(static_cast<uint32_t>(i), static_cast<uint32_t>(i + 1),
				static_cast<uint32_t>(i + 2));
		}
	} else {
		for (size_t i = 0; i + 2 < indices.size(); i += 3) {
			tri(indices[i], indices[i + 1], indices[i + 2]);
		}
	}
}

} // namespace rig
