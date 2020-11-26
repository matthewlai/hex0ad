#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <memory>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat4x4.hpp"

#include "graphics_settings.h"
#include "shaders.h"

class Renderable {
 public:
  struct RenderContext {
    uint64_t frame_counter;
    glm::mat4 vp;
    glm::vec3 light_pos;
    glm::vec3 eye_pos;

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
};

template <typename RenderableIterator>
void Renderer::Render(RenderableIterator begin, RenderableIterator end) {
  SDL_Window* window = SDL_GL_GetCurrentWindow();
  int window_width;
  int window_height;
  SDL_GL_GetDrawableSize(window, &window_width, &window_height);

  glViewport(0, 0, window_width, window_height);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  float camera_angle_rad = render_context_.frame_counter / 200.0f;

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

  ++render_context_.frame_counter;
}

#endif // RENDERER_H

