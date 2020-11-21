#ifndef PLATFORM_INCLUDES_H
#define PLATFORM_INCLUDES_H

#if defined(_WIN32)
#define HAVE_GLEW
#include "GL/glew.h"
#endif

#include <SDL.h>

// OSX doesn't support OpenGL ES. We use OpenGL ES on all other
// platforms.
#if defined(__APPLE__) && defined(__MACH__)
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#else
#include <SDL_opengles2.h>
#endif

// We target OpenGL 4.1 on Mac because ES is not supported.
#if defined(__APPLE__) && defined(__MACH__)
#define USE_OPENGL
#endif

#endif // PLATFORM_INCLUDES_H