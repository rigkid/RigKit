#include "rendering/FpsOverlay.h"

#include "rendering/IRenderer.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace rigkit {
namespace {

int g_shown = 0;
int g_frames = 0;
float g_age = 0.f;

uint16_t glyphBits(char c) {
	switch (c) {
	case '0':
		return 0x7b6f;
	case '1':
		return 0x749a;
	case '2':
		return 0x73e7;
	case '3':
		return 0x71e7;
	case '4':
		return 0x49ed;
	case '5':
		return 0x79cf;
	case '6':
		return 0x7bcf;
	case '7':
		return 0x4927;
	case '8':
		return 0x7bef;
	case '9':
		return 0x79ef;
	case 'f':
		return 0x13cf;
	case 'p':
		return 0x13ef;
	case 's':
		return 0x79cf;
	default:
		return 0;
	}
}

void drawFpsBitmap(IRenderer& r, const char* buf, float x0, float y0) {
	const float scale = 2.f;
	const float gap = 2.f;
	const float pad = 6.f;
	const float glyphW = 3.f * scale;
	const float glyphH = 5.f * scale;
	const size_t n = std::strlen(buf);
	const float boxW = pad * 2.f + static_cast<float>(n) * (glyphW + gap) - gap;
	const float boxH = pad * 2.f + glyphH;
	r.drawRect(x0, y0, boxW, boxH, Paint::fill({0.02f, 0.02f, 0.03f, 0.72f}));

	std::vector<glm::vec2> tris;
	tris.reserve(n * 15 * 6);
	float x = x0 + pad;
	const float y = y0 + pad;
	for (size_t i = 0; i < n; ++i) {
		const uint16_t bits = glyphBits(buf[i]);
		for (int row = 0; row < 5; ++row) {
			for (int col = 0; col < 3; ++col) {
				if ((bits & (1u << (row * 3 + col))) == 0) {
					continue;
				}
				const float px = x + static_cast<float>(col) * scale;
				const float py = y + static_cast<float>(row) * scale;
				tris.push_back({px, py});
				tris.push_back({px + scale, py});
				tris.push_back({px + scale, py + scale});
				tris.push_back({px, py});
				tris.push_back({px + scale, py + scale});
				tris.push_back({px, py + scale});
			}
		}
		x += glyphW + gap;
	}
	if (!tris.empty()) {
		r.drawTriangles(tris, Paint::fill({0.92f, 0.93f, 0.94f, 1.f}));
	}
}

void drawFps(IRenderer& r, int fps) {
	char buf[16];
	std::snprintf(buf, sizeof(buf), "%d fps", fps);
	const float x0 = 24.f;
	const float y0 = 16.f;
	const glm::vec4 ink{0.92f, 0.93f, 0.94f, 1.f};
	if (r.hasFilledText()) {
		r.setFont("RobotoFlex", 16.f);
		const glm::vec2 b = r.getTextBounds(buf);
		if (b.x > 1.f && b.y > 1.f) {
			const float pad = 6.f;
			r.drawRect(x0, y0, b.x + pad * 2.f, b.y + pad * 2.f,
					   Paint::fill({0.02f, 0.02f, 0.03f, 0.72f}));
			r.drawText(buf, x0 + pad, y0 + pad, ink);
			return;
		}
	}
	drawFpsBitmap(r, buf, x0, y0);
}

} // namespace

void presentFpsOverlay(IRenderer& renderer, int instantFps, float dt) {
	if (dt > 0.f) {
		++g_frames;
		g_age += dt;
		if (g_age >= 0.25f) {
			g_shown = static_cast<int>(static_cast<float>(g_frames) / g_age + 0.5f);
			g_age = 0.f;
			g_frames = 0;
		}
	}
	if (g_shown <= 0) {
		g_shown = instantFps;
	}
	if (g_shown <= 0) {
		return;
	}
	renderer.resetMatrix();
	drawFps(renderer, g_shown);
}

} // namespace rigkit
