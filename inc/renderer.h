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
    uint64_t frame_start_time;

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

  void RenderFrameBegin();
  void Render(Renderable* renderable);
  void RenderFrameEnd();

  #define GraphicsSetting(upper, lower, type, default, toggle_key) \
    void Toggle ## upper() { render_context_.lower ^= 0x1; }
    GRAPHICS_SETTINGS
  #undef GraphicsSetting

 private:
  Renderable::RenderContext render_context_;
  uint64_t last_stat_time_us_;
};

#endif // RENDERER_H

