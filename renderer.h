#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <memory>
#include <stack>

#include "glm/mat4x4.hpp"

#include "shaders.h"

class Renderer {
 public:
  Renderer();

  void Render();

  void PushModelMatrix() { model_stack_.push(model_); }
  void PopModelMatrix() {
  	model_ = model_stack_.top();
  	model_stack_.pop();
  }

 private:
  std::unique_ptr<ShaderProgram> simple_shader_;
  GLint simple_shader_mvp_loc_;

  glm::mat4 projection_;
  glm::mat4 view_;
  glm::mat4 model_;

  std::stack<glm::mat4x4> model_stack_;

  uint64_t frame_counter_;
};

#endif // RENDERER_H

