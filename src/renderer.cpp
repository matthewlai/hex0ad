#include "renderer.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include "platform_includes.h"

#include <cstddef>
#include <iostream>

TestTriangleRenderable::TestTriangleRenderable() {
  const static GLfloat vertices[] = { 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f,
                                      0.0f,  -1.0f, 0.0f, };
  const static GLuint indices[] = {
    0, 1, 2,
    2, 1, 0
  };

  simple_shader_ = GetShader("shaders/simple.vs", "shaders/simple.fs");
  simple_shader_mvp_loc_ = simple_shader_->GetUniformLocation("mvp");
  glGenBuffers(1, &vertices_vbo_id_);
  glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_id_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 3, vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &indices_vbo_id_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 3 * 2, indices, GL_STATIC_DRAW);
}

TestTriangleRenderable::~TestTriangleRenderable() {
  glDeleteBuffers(1, &vertices_vbo_id_);
  glDeleteBuffers(1, &indices_vbo_id_);
}

void TestTriangleRenderable::Render(uint64_t frame_counter, const glm::mat4& vp) {
  glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(float(frame_counter % 360)),
    glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 mvp = vp * model;
  glUniformMatrix4fv(simple_shader_mvp_loc_, 1, GL_FALSE, glm::value_ptr(mvp));

  simple_shader_->Activate();
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_id_);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*) 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id_);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);

  glDisableVertexAttribArray(0);
}

Renderer::Renderer() {
  frame_counter_ = 0;

  #ifdef USE_OPENGL
  // OpenGL core profile doesn't have a default VAO. Use a single VAO for now.
  unsigned int vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao); 
  #endif
}
