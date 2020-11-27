#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "platform_includes.h"

#include "actor.h"
#include "logger.h"
#include "renderer.h"
#include "utils.h"

namespace {
const static int kScreenWidth = 1920;
const static int kScreenHeight = 1080;

struct ProgramState {
  SDL_Window* window;
  std::unique_ptr<Renderer> renderer;
  std::vector<Actor> actors;
} g_state;

SDL_GLContext InitSDL() {
  CHECK_SDL_ERROR(SDL_Init(SDL_INIT_VIDEO));
  
  // No context attributes in Emscripten. We should always get the right one on
  // Emscripten anyways.
  #ifndef __EMSCRIPTEN__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  #endif

  // OpenGL 4.1 on Mac.
  #if defined(__APPLE__) && defined(__MACH__)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  #endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  g_state.window = SDL_CreateWindow(
      "hex0ad", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      kScreenWidth, kScreenHeight, SDL_WINDOW_OPENGL);
  CHECK_SDL_ERROR_PTR(g_state.window);
  
  auto context = SDL_GL_CreateContext(g_state.window);

#ifdef HAVE_GLEW
  if (glewInit() != GLEW_OK) {
    throw std::runtime_error("Failed to initialize glew");
  }
#endif
  
  CHECK_SDL_ERROR_PTR(context);
  
  const auto* version = glGetString(GL_VERSION);
  
  if (!version) {
    LOG_ERROR("Failed to create OpenGL ES 3.0 context.");
    throw std::runtime_error("Failed to create OpenGL ES 3.0 context.");
  }
  
  LOG_INFO("OpenGL ES Platform info:");
  LOG_INFO("Vendor: %", glGetString(GL_VENDOR));
  LOG_INFO("Renderer: %", glGetString(GL_RENDERER));

#ifdef __EMSCRIPTEN__
  if (SDL_GL_ExtensionSupported("WEBGL_debug_renderer_info")) {
    const std::string unmasked_vendor = emscripten_run_script_string(
        "var gl = document.createElement('canvas').getContext('webgl2');"
        "var debugInfo = gl.getExtension('WEBGL_debug_renderer_info');"
        "gl.getParameter(debugInfo.UNMASKED_VENDOR_WEBGL);");

    const std::string unmasked_renderer = emscripten_run_script_string(
        "var gl = document.createElement('canvas').getContext('webgl2');"
        "var debugInfo = gl.getExtension('WEBGL_debug_renderer_info');"
        "gl.getParameter(debugInfo.UNMASKED_RENDERER_WEBGL);");

    LOG_INFO("Hardware Vendor: %", unmasked_vendor);
    LOG_INFO("Hardware Renderer: %", unmasked_renderer);
  }
#endif

  LOG_INFO("Version: %", glGetString(GL_VERSION));
  LOG_INFO("Shading language version: %",
           glGetString(GL_SHADING_LANGUAGE_VERSION));

  // First try to enable adaptive vsync.
  if (SDL_GL_SetSwapInterval(-1) == 0) {
    LOG_INFO("Using adaptive vsync");
  } else {
    LOG_WARN("Failed to enable adaptive vsync: %", SDL_GetError());
    
    // If that didn't work, try standard vsync.
    if (SDL_GL_SetSwapInterval(1) == 0) {
      LOG_INFO("Using vsync");
    } else {
      LOG_WARN("Failed to enable vsync: %", SDL_GetError());
    }
  }

  return context;
}

bool main_loop() {
  SDL_Event e;
  bool quit = false;
  while (SDL_PollEvent(&e) != 0) {
    if (e.type == SDL_QUIT) {
      quit = true;
    } else if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.sym) {
        #define GraphicsSetting(upper, lower, type, default, toggle_key) \
        case toggle_key: \
          g_state.renderer->Toggle ## upper (); \
          break;
        GRAPHICS_SETTINGS
        #undef GraphicsSetting
        case SDLK_ESCAPE:
          quit = true;
          break;
        default:
          // Unknown key.
          break;
      }
    }
  }

  static TestTriangleRenderable tri_renderable;

  //g_state.renderer->Render(&tri_renderable);

  for (auto& actor : g_state.actors) {
    g_state.renderer->Render(&actor);
  }

  SDL_GL_SwapWindow(g_state.window);
  return quit;
}

void empty_loop() {}

void emscripten_main_loop() {
  main_loop();
}

void DeInitSDL() {
  SDL_DestroyWindow(g_state.window);
  g_state.window = nullptr;
  SDL_Quit();
}
}

int main(int /*argc*/, char** /*argv*/) {
  logger.LogToStdOutLevel(Logger::eLevel::INFO);

  #ifdef __EMSCRIPTEN__
  int have_webgl2 = emscripten_run_script_int(R""(
    try { gl = canvas.getContext("webgl2"); } catch (x) { gl = null; } gl != null;)"");
  if (!have_webgl2) {
    LOG_ERROR("WebGL 2 not supported by your browser.");
    LOG_ERROR("See https://caniuse.com/webgl2 for supported browser versions.");
    emscripten_set_main_loop(empty_loop, 0, 1);
  }
  #endif

  SDL_GLContext context = InitSDL();
  (void) context;

  #ifdef __EMSCRIPTEN__
  LOG_INFO("User Agent: %",
           emscripten_run_script_string("navigator.userAgent"));
  #endif

  g_state.renderer = std::make_unique<Renderer>();

  //ActorTemplate fortress("structures/persians/fortress.fb");
  ActorTemplate fortress("structures/romans/fortress.fb");

  g_state.actors.push_back(fortress.MakeActor());

  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(emscripten_main_loop, 0, 1);
  #else
  while (!main_loop()) {}
  #endif
  // Anything after this is never executed in emscripten mode.

  DeInitSDL();
  
  return 0;
}
