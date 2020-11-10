#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <GLES3/gl3.h>
#include <GLES3/gl2ext.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "logger.h"
#include "renderer.h"
#include "utils.h"

namespace {
const static int kScreenWidth = 800;
const static int kScreenHeight = 600;

SDL_Window* g_window;
std::unique_ptr<Renderer> g_renderer;

SDL_GLContext InitSDL() {
  CHECK_SDL_ERROR(SDL_Init(SDL_INIT_VIDEO));
  
  // No context attributes in Emscripten. We should always get the right one on
  // Emscripten anyways.
  #ifndef __EMSCRIPTEN__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  #endif

  g_window = SDL_CreateWindow(
      "TessWar", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      kScreenWidth, kScreenHeight, SDL_WINDOW_OPENGL);
  CHECK_SDL_ERROR_PTR(g_window);
  
  auto context = SDL_GL_CreateContext(g_window);
  
  CHECK_SDL_ERROR_PTR(context);
  
  const auto* version = glGetString(GL_VERSION);
  
  if (!version) {
    LOG_ERROR("Failed to create OpenGL ES 3.0 context.");
    throw std::runtime_error("Failed to create OpenGL ES 3.0 context.");
  }
  
  LOG_INFO("OpenGL ES Platform info:");
  LOG_INFO("Vendor: %", glGetString(GL_VENDOR));
  LOG_INFO("Renderer: %", glGetString(GL_RENDERER));

  if (SDL_GL_ExtensionSupported("WEBGL_debug_renderer_info")) {
    const std::string unmasked_vendor = emscripten_run_script_string(
        "var gl = document.createElement('canvas').getContext('webgl');"
        "var debugInfo = gl.getExtension('WEBGL_debug_renderer_info');"
        "gl.getParameter(debugInfo.UNMASKED_VENDOR_WEBGL);");

    const std::string unmasked_renderer = emscripten_run_script_string(
        "var gl = document.createElement('canvas').getContext('webgl');"
        "var debugInfo = gl.getExtension('WEBGL_debug_renderer_info');"
        "gl.getParameter(debugInfo.UNMASKED_RENDERER_WEBGL);");

    LOG_INFO("Hardware Vendor: %", unmasked_vendor);
    LOG_INFO("Hardware Renderer: %", unmasked_renderer);
  }

  LOG_INFO("Version: %", glGetString(GL_VERSION));
  LOG_INFO("Shading language version: %",
           glGetString(GL_SHADING_LANGUAGE_VERSION));
           
  if (SDL_GL_SetSwapInterval(1) < 0) {
		LOG_WARN("Failed to enable vsync: %", SDL_GetError());
	}

  return context;
}

void DeInitSDL() {
  SDL_DestroyWindow(g_window);
  g_window = nullptr;
  SDL_Quit();
}

bool main_loop() {
  SDL_Event e;
  bool quit = false;
  while (SDL_PollEvent(&e) != 0) {
    if (e.type == SDL_QUIT) {
      quit = true;
    } else if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.sym) {
        case SDLK_ESCAPE: {
          quit = true;
        }
        default: {
          // Unknown key.
        }
      }
    }
  }

  g_renderer->Render();
  SDL_GL_SwapWindow(g_window);
  return quit;
}

void emscripten_main_loop() {
  main_loop();
}
}

int main(int /*argc*/, char** /*argv*/) {
  logger.LogToStdOutLevel(Logger::INFO);
  SDL_GLContext context = InitSDL();

  #ifdef __EMSCRIPTEN__
  LOG_INFO("User Agent: %",
           emscripten_run_script_string("navigator.userAgent"));
  #endif

  g_renderer = std::make_unique<Renderer>();

  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(emscripten_main_loop, 0, 1);
  #else
  while (!main_loop()) {}
  #endif
  // Anything after this is never executed in emscripten mode.

  DeInitSDL();
  
  return 0;
}
