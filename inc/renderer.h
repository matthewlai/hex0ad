#ifndef RENDERER_H
#define RENDERER_H

#include <chrono>
#include <cstdint>
#include <memory>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat4x4.hpp"

#include "graphics_settings.h"
#include "shaders.h"
#include "utils.h"

namespace {
// How often rendering stats are updated.
constexpr uint64_t kRenderStatsPeriod = 1000000;
}

class Renderable {
 public:
  struct RenderContext {
    uint64_t frame_counter;
    glm::mat4 vp;
    glm::vec3 light_pos;
    glm::vec3 eye_pos;

    uint64_t last_frame_time_us;

    #define GraphicsSetting(upper, lower, type, default, toggle_key) type lower;
    GRAPHICS_SETTINGS
    #undef GraphicsSetting
  };
  virtual void Render(RenderContext* context) = 0;
};

class TestTriangleRenderable : public Renderable {
 public:
  TestTriangleRenderable();
  virtual ~TestTriangleRenderable();
  void Render(RenderContext* context) override;

 private:
  ShaderProgram* simple_shader_;
  GLint simple_shader_mvp_loc_;
  GLuint vertices_vbo_id_;
  GLuint indices_vbo_id_;
};

class Renderer {
 public:
  Renderer();

  template <typename RenderableIterator>
  void Render(RenderableIterator begin, RenderableIterator end);

  template <typename Renderable>
  void Render(Renderable* renderable) { Render(renderable, renderable + 1); }

  #define GraphicsSetting(upper, lower, type, default, toggle_key) \
    void Toggle ## upper() { render_context_.lower ^= 0x1; }
    GRAPHICS_SETTINGS
  #undef GraphicsSetting

 private:
  Renderable::RenderContext render_context_;
  uint64_t last_stat_time_us_;
};

template <typename RenderableIterator>
void Renderer::Render(RenderableIterator begin, RenderableIterator end) {
  // Make sure the last frame swap is done. This doesn't result in much
  // performance penalty because with double buffering (as opposed to triple
  // buffering), most GL calls we make below will require the swap to be done
  // anyways. Calling glFinish() here allows us to get a better time measurement.
  // All non-graphics CPU work should have been done by this point.
  glFinish();

  uint64_t frame_start_time = GetTimeUs();

  SDL_Window* window = SDL_GL_GetCurrentWindow();
  int window_width;
  int window_height;
  SDL_GL_GetDrawableSize(window, &window_width, &window_height);

  glViewport(0, 0, window_width, window_height);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  double camera_angle_rad = frame_start_time / 3000000.0;

  float eye_x = 30.0f * sin(camera_angle_rad);
  float eye_y = 30.0f * cos(camera_angle_rad);

  render_context_.eye_pos = glm::vec3(eye_x, eye_y, 20.0f);

  glm::mat4 view = glm::lookAt(render_context_.eye_pos, glm::vec3(0.0f, 0.0f, 5.0f),
    glm::vec3(0.0f, 0.0f, 1.0f));

  glm::mat4 projection = glm::perspective(glm::radians(90.0f),
    static_cast<float>(window_width) / window_height,
    1.0f, 100.0f);

  render_context_.vp = projection * view;

  // Put the light 45 degrees from eye.
  render_context_.light_pos = glm::vec3(30.0f * sin(camera_angle_rad + M_PI / 4.0f),
                                        30.0f * cos(camera_angle_rad + M_PI / 4.0f), 7.0f);

  for (auto it = begin; it != end; ++it) {
    it->Render(&render_context_);
  }

  // This is when all the draw calls have been issued (all the CPU work is done).
  uint64_t draw_calls_end_time = GetTimeUs();

  glFinish();

  // This is when all the drawing is done (to the back buffer).
  uint64_t frame_end_time = GetTimeUs();

  ++render_context_.frame_counter;
  if ((frame_end_time - last_stat_time_us_) > kRenderStatsPeriod) {
    uint64_t time_since_last_frame_us = frame_end_time - render_context_.last_frame_time_us;
    float frame_rate = 1000000.0f / time_since_last_frame_us;
    LOG_INFO("Frame rate: %, draw calls time: % us, render time: % us",
             frame_rate,
             (draw_calls_end_time - frame_start_time),
             (frame_end_time - frame_start_time));
    last_stat_time_us_ = frame_end_time;
  }
  render_context_.last_frame_time_us = frame_end_time;
}

#endif // RENDERER_H

