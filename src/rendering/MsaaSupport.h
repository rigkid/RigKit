#pragma once

#include "rendering/U_gladGlfw.h"

namespace rigkit {
namespace msaa {

inline int maxSamples() {
	GLint m = 0;
#if defined(RIGKIT_GLES)
	if (GLAD_GL_ANGLE_framebuffer_multisample) {
		glGetIntegerv(GL_MAX_SAMPLES_ANGLE, &m);
	} else if (GLAD_GL_APPLE_framebuffer_multisample) {
		glGetIntegerv(GL_MAX_SAMPLES_APPLE, &m);
	} else if (GLAD_GL_EXT_multisampled_render_to_texture) {
		glGetIntegerv(GL_MAX_SAMPLES_EXT, &m);
	} else if (GLAD_GL_NV_framebuffer_multisample) {
		glGetIntegerv(GL_MAX_SAMPLES_NV, &m);
	}
#else
	glGetIntegerv(GL_MAX_SAMPLES, &m);
#endif
	return m > 0 ? m : 0;
}

inline bool canBlit() {
#if defined(RIGKIT_GLES)
	return (GLAD_GL_ANGLE_framebuffer_multisample && GLAD_GL_ANGLE_framebuffer_blit) ||
		   GLAD_GL_APPLE_framebuffer_multisample ||
		   (GLAD_GL_NV_framebuffer_multisample && GLAD_GL_NV_framebuffer_blit);
#else
	return true;
#endif
}

inline bool canExtToTexture() {
#if defined(RIGKIT_GLES)
	return GLAD_GL_EXT_multisampled_render_to_texture != 0;
#else
	return false;
#endif
}

/// Clamp a requested count. 0/1 stay 1. Missing GLES resolve stays 1.
inline int clampSamples(int want) {
	if (want <= 1) {
		return 1;
	}
	if (!canBlit() && !canExtToTexture()) {
		return 1;
	}
	const int cap = maxSamples();
	if (cap <= 1) {
		return 1;
	}
	return want > cap ? cap : want;
}

inline bool fboComplete() {
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

inline void makeColorTexture(GLuint& tex, int width, int height, bool clampToEdge = false) {
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (clampToEdge) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}

inline void storeColor(GLsizei samples, int width, int height) {
#if defined(RIGKIT_GLES)
	if (GLAD_GL_ANGLE_framebuffer_multisample) {
		glRenderbufferStorageMultisampleANGLE(GL_RENDERBUFFER, samples, GL_RGBA8_OES, width,
											  height);
	} else if (GLAD_GL_APPLE_framebuffer_multisample) {
		glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, samples, GL_RGBA8_OES, width,
											  height);
	} else if (GLAD_GL_NV_framebuffer_multisample) {
		glRenderbufferStorageMultisampleNV(GL_RENDERBUFFER, samples, GL_RGBA8_OES, width, height);
	}
#else
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
#endif
}

inline void storeDepth(GLsizei samples, int width, int height) {
#if defined(RIGKIT_GLES)
	if (GLAD_GL_ANGLE_framebuffer_multisample) {
		glRenderbufferStorageMultisampleANGLE(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width,
											  height);
	} else if (GLAD_GL_APPLE_framebuffer_multisample) {
		glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width,
											  height);
	} else if (GLAD_GL_EXT_multisampled_render_to_texture) {
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width,
											height);
	} else if (GLAD_GL_NV_framebuffer_multisample) {
		glRenderbufferStorageMultisampleNV(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width,
										   height);
	}
#else
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
#endif
}

inline void blitResolve(GLuint src, GLuint dst, int width, int height) {
#if defined(RIGKIT_GLES)
	if (GLAD_GL_ANGLE_framebuffer_blit) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER_ANGLE, src);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER_ANGLE, dst);
		glBlitFramebufferANGLE(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
							   GL_LINEAR);
	} else if (GLAD_GL_APPLE_framebuffer_multisample) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, src);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, dst);
		glResolveMultisampleFramebufferAPPLE();
	} else if (GLAD_GL_NV_framebuffer_blit) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER_NV, src);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER_NV, dst);
		glBlitFramebufferNV(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
							GL_LINEAR);
	}
#else
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
#endif
}

#if defined(RIGKIT_GLES)
inline void attachColorExt(GLuint tex, GLsizei samples) {
	glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex,
										 0, samples);
}
#endif

} // namespace msaa
} // namespace rigkit
