#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <memory>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat4x4.hpp"

#include "shaders.h"

class Renderable {
 public:
  virtual void Render(uint64_t frame_counter, const glm::mat4& vp) = 0;
};

class TestTriangleRenderable : public Renderable {
 public:
  TestTriangleRenderable();
  virtual ~TestTriangleRenderable();
  void Render(uint64_t frame_counter, const glm::mat4& vp) override;

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

 private:
  uint64_t frame_counter_;
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
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glm::mat4 view = glm::lookAt(glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

  glm::mat4 projection = glm::perspective(glm::radians(90.0f),
    static_cast<float>(window_width) / window_height,
    1.0f, 100.0f);

  glm::mat4 vp = projection * view;

  for (auto it = begin; it != end; ++it) {
    it->Render(frame_counter_, vp);
  }

  ++frame_counter_;
}

#endif // RENDERER_H

