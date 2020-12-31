#ifndef UI_H
#define UI_H

#include <string>
#include <vector>

#include "platform_includes.h"
#include "renderer.h"

class UI : public Renderable {
 public:
  UI();

  void Render(RenderContext* context) override;

  void SetDebugText(std::size_t line, const std::string& text) {
    if (debug_text_.size() <= line) {
      debug_text_.resize(line + 1);
    }
    debug_text_[line] = text;
  }

  virtual ~UI() {}

 private:
  void DrawImage(SDL_Surface* surface, float x, float y, float w, float h);

  ShaderProgram* shader_;
  bool initialized_;
  GLuint vertices_vbo_id_;
  GLuint indices_vbo_id_;

  std::vector<std::string> debug_text_;

  TTF_Font* font_;
  GLuint texture_;
};

#endif // UI_H
