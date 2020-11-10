#include "renderer.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include <cstddef>
#include <iostream>

Renderer::Renderer() {
  simple_shader_ = std::make_unique<ShaderProgram>("shaders/simple.vs",
                                                   "shaders/simple.fs");
  simple_shader_->Activate();
  simple_shader_mvp_loc_ = simple_shader_->GetUniformLocation("mvp");

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_CULL_FACE);

  model_ = glm::mat4(1.0);
  view_ = glm::mat4(1.0);
  projection_ = glm::mat4(1.0);

  frame_counter_ = 0;
}

void Renderer::Render() {
  SDL_Window* window = SDL_GL_GetCurrentWindow();
  int window_width;
  int window_height;
  SDL_GL_GetDrawableSize(window, &window_width, &window_height);

  glViewport(0, 0, window_width, window_height);

  const static GLfloat vertices[] = { 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f,
                                      0.0f,  -1.0f, 0.0f,
                                      0.0f,  -1.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f,
                                      0.0f, 1.0f, 0.0f };

  glClear(GL_COLOR_BUFFER_BIT);
  simple_shader_->Activate();
  
  GLuint vbo_ids[1];
  glGenBuffers(1, vbo_ids);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 6, vertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  
  std::size_t offset = 0;
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*) offset);

  view_ = glm::lookAt(glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, 1.0f, 0.0f));

  projection_ = glm::perspective(glm::radians(90.0f),
                                 static_cast<float>(window_width) / window_height,
                                 1.0f, 100.0f);

  PushModelMatrix();
  model_ = glm::rotate(model_, glm::radians(float(frame_counter_ % 360)),
                       glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 mvp = projection_ * view_ * model_;

  glUniformMatrix4fv(simple_shader_mvp_loc_, 1, GL_FALSE, glm::value_ptr(mvp));
  PopModelMatrix();

  glDrawArrays(GL_TRIANGLES, 0, 6);
  
  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  ++frame_counter_;
}

