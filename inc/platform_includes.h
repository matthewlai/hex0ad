#ifndef PLATFORM_INCLUDES_H
#define PLATFORM_INCLUDES_H

#if defined(_WIN32)
#define HAVE_GLEW
#include "GL/glew.h"
#endif

#if defined(_WIN32)
// We are not using sdl2-config on Windows
#include <SDL2/SDL.h>
#else
// On other platforms sdl2-config will make sure we can find
// SDL.h
#include <SDL.h>
#endif

// OSX doesn't support OpenGL ES. We use OpenGL ES on all other
// platforms.
#if defined(__APPLE__) && defined(__MACH__)
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#elif !defined(_WIN32)
// This seems to bring in GLES3 on other platforms?
#include <SDL_opengles2.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

#include <GLES3/gl31.h>
#endif

// We target OpenGL 4.1 on Mac because ES is not supported.
#if defined(__APPLE__) && defined(__MACH__)
#define USE_OPENGL
#endif

#endif // PLATFORM_INCLUDES_H