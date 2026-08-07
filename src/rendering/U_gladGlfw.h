#pragma once

// GLAD first, then GLFW without system GL headers.
#if defined(RIGKIT_GLES)
#include <glad/gles2.h>
// GLES2 core lacks VAO / packed depth-stencil — use OES entry points.
#ifndef glGenVertexArrays
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#define glDeleteVertexArrays glDeleteVertexArraysOES
#define glIsVertexArray glIsVertexArrayOES
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#endif
// Packed depth-stencil attachment (ES3 / common OES implementations); not in core ES2 headers.
#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#endif
#else
#include <glad/gl.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
