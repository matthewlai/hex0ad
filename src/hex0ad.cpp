#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "platform_includes.h"

#include "actor.h"
#include "logger.h"
#include "renderer.h"
#include "resources.h"
#include "terrain.h"
#include "ui.h"
#include "utils.h"

namespace {
const static int kScreenWidth = 1920;
const static int kScreenHeight = 1080;

const static int64_t kFrameRateReportIntervalUs = 330000;

struct ProgramState {
  SDL_Window* window;
  bool fullscreen = false;
  std::unique_ptr<Renderer> renderer;
  std::unique_ptr<Terrain> terrain;
  std::vector<Actor> actors;
  std::unique_ptr<UI> ui;
  int32_t last_mouse_x;
  int32_t last_mouse_y;

  bool vsync_on = false;
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

  #ifdef HAVE_GLEW
  // Try to get debug context if we can.
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
  #endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

  g_state.window = SDL_CreateWindow(
      "hex0ad", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      kScreenWidth, kScreenHeight, window_flags);
  CHECK_SDL_ERROR_PTR(g_state.window);
  
  auto context = SDL_GL_CreateContext(g_state.window);

#ifdef HAVE_GLEW
  if (glewInit() != GLEW_OK) {
    throw std::runtime_error("Failed to initialize glew");
  }

  GLint context_flags;
  glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
  if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
    LOG_INFO("GL Context has debug flag, hooking up callback");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(GlDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
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

  return context;
}

bool main_loop() {
  static uint64_t last_frame_rate_report = GetTimeUs();
  static uint64_t frames_since_last_report = 0;

  uint64_t current_time_us = GetTimeUs();

  // World updates.
  for (auto& actor : g_state.actors) {
    actor.Update(current_time_us);
  }

  int mouse_x;
  int mouse_y;
  SDL_GetMouseState(&mouse_x, &mouse_y);
  glm::vec3 mouse_world_pos = g_state.renderer->UnProjectToXY(mouse_x, mouse_y);
  Hex hex = g_state.terrain->CoordsToHex(glm::vec2(mouse_world_pos.x, mouse_world_pos.y));

  g_state.ui->SetDebugText(1, FormatString("(x=%, y=%) (r=%, s=%, q=%)", mouse_world_pos.x, mouse_world_pos.y,
                           hex.r(), hex.s(), hex.q()));

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
        case SDLK_g:
          g_state.terrain->ToggleRenderGround();
          break;
        case SDLK_ESCAPE:
          quit = true;
          break;
#ifndef __EMSCRIPTEN__
        case SDLK_F11:
          g_state.fullscreen ^= 1;
          if (g_state.fullscreen) {
            SDL_SetWindowFullscreen(g_state.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
          } else {
            SDL_SetWindowFullscreen(g_state.window, 0);
          }
#endif
        default:
          // Unknown key.
          break;
      }
    } else if (e.type == SDL_WINDOWEVENT) {
      if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
        LOG_INFO("Window resized to %x%", e.window.data1, e.window.data2);
        SDL_SetWindowSize(g_state.window, e.window.data1, e.window.data2);
      }
    } else if (e.type == SDL_MOUSEMOTION) {
      if (e.motion.state & SDL_BUTTON_LMASK) {
        g_state.renderer->AddAzimuth(e.motion.xrel * 0.1f);
        g_state.renderer->AddElevation(e.motion.yrel * 0.1f);
      } else if (e.motion.state & SDL_BUTTON_RMASK) {
        g_state.renderer->MoveCamera(e.motion.x - e.motion.xrel,
                                     e.motion.y - e.motion.yrel,
                                     e.motion.x, e.motion.y);
      }
    } else if (e.type == SDL_MOUSEWHEEL) {
      int scroll_amount = e.wheel.y;
      if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
        scroll_amount *= -1;
      }

      // Portable mousewheel handling is hard on emscripten:
      // https://github.com/emscripten-core/emscripten/issues/6283
      // We just use 1 instead, which seems to work fine.
      scroll_amount = (scroll_amount > 0) ? 1 : -1;

      g_state.renderer->AddDistance(scroll_amount * 0.2f);
    }
  }

  std::vector<Renderable*> renderables;

  for (auto& actor : g_state.actors) {
    renderables.push_back(&actor);
  }
  renderables.push_back(g_state.terrain.get());
  renderables.push_back(g_state.ui.get());

  g_state.renderer->RenderFrame(renderables);

  ++frames_since_last_report;

  uint64_t time_now = GetTimeUs();
  int64_t elapsed = time_now - last_frame_rate_report;
  if (elapsed > kFrameRateReportIntervalUs) {
    double avg_frame_time_ms = static_cast<double>(elapsed) / frames_since_last_report / 1000.0;
    g_state.ui->SetDebugText(0, FormatString("Avg Frame Time: % ms (% FPS)", avg_frame_time_ms, 1000.0 / avg_frame_time_ms));
    frames_since_last_report = 0;
    last_frame_rate_report = time_now;
  }

  if (g_state.vsync_on && !g_state.renderer->UseVsync()) {
    SDL_GL_SetSwapInterval(0);
    g_state.vsync_on = false;
    LOG_INFO("Vsync off");
  } else if (!g_state.vsync_on && g_state.renderer->UseVsync()) {
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
    g_state.vsync_on = true;
  }

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
  logger.LogToStdErrLevel(Logger::eLevel::WARN);
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

  g_state.terrain = std::make_unique<Terrain>();

  g_state.ui = std::make_unique<UI>();

  for (const auto& path : kTestActorPaths) {
    g_state.actors.push_back(ActorTemplate::GetTemplate(std::string(path)).MakeActor());
    if (std::string(path).find("units") != std::string::npos) {
      g_state.actors.rbegin()->SetScale(3.0f);
    }
  }

  for (std::size_t i = 0; i < g_state.actors.size(); ++i) {
    float arg = 2.0f * M_PI / g_state.actors.size() * i;
    float dist = (g_state.actors.size() - 1) * 4.0f;
    glm::vec2 position(dist * cos(arg), dist * sin(arg));
    position = g_state.terrain->SnapToGrid(position);
    g_state.actors[i].SetPosition(glm::vec3(position.x, position.y, 0.0f));
    g_state.actors[i].SetRotationRad(arg + 0.5f * M_PI);
  }

  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(emscripten_main_loop, 0, 1);
  #else
  while (!main_loop()) {}
  #endif
  // Anything after this is never executed in emscripten mode.

  DeInitSDL();
  
  return 0;
}
