#include "ui.h"

#include <cstring>
#include <sstream>

#include "glm/gtx/string_cast.hpp"

#include "logger.h"
#include "platform_includes.h"
#include "renderer.h"
#include "shaders.h"
#include "texture_manager.h"
#include "utils.h"

namespace {
constexpr static const char* kFontPath = "assets/fonts/RobotoMono-Regular.ttf";

// Higher font size = higher resolution rendering.
constexpr static int kFontSize = 40;
constexpr static float kLineHeight = 0.02f;
constexpr static float kLineSpacing = 0.018f;
constexpr static float kLineXOffset = 0.003f;
constexpr static float kLineYOffset = 0.0f;

// 2048x2048 is the maximum for OpenGL ES 3 (and 99.9% support
// 4096x4096).
constexpr static int kGpuTextureWidth = 2048;
constexpr static int kGpuTextureHeight = 2048;
}

UI::UI() : 
    shader_(nullptr),
    initialized_(false),
    vertices_vbo_id_(0),
    indices_vbo_id_(0) {}

void UI::Render(RenderContext* context) {
  if (!initialized_) {
    std::vector<float> positions;
    positions.push_back(0.0f); positions.push_back(0.0f); // v0
    positions.push_back(0.0f); positions.push_back(1.0f); // v1
    positions.push_back(1.0f); positions.push_back(0.0f); // v2
    positions.push_back(1.0f); positions.push_back(1.0f); // v3

    std::vector<GLuint> indices;
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(3); indices.push_back(2); indices.push_back(1);

    vertices_vbo_id_ = MakeAndUploadVBO(GL_ARRAY_BUFFER, positions);
    indices_vbo_id_ = MakeAndUploadVBO(GL_ELEMENT_ARRAY_BUFFER, indices);

    texture_ = TextureManager::GetInstance()->MakeStreamingTexture(
      kGpuTextureWidth, kGpuTextureHeight);

    shader_ = GetShader("shaders/ui.vs", "shaders/ui.fs");

    if (TTF_Init() < 0) {
      LOG_ERROR("Failed to initialize SDL_ttf: %", TTF_GetError());
    }

    font_ = TTF_OpenFont(kFontPath, kFontSize);
    if (!font_) {
      LOG_ERROR("Failed to open font: %", TTF_GetError());
    }

    initialized_ = true;
  }

  shader_->Activate();

  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  for (std::size_t i = 0; i < debug_text_.size(); ++i) {
    if (debug_text_[i].empty()) {
      continue;
    }

    SDL_Surface* text = TTF_RenderText_Blended(font_, debug_text_[i].c_str(), SDL_Color{255, 255, 255, 255});

    if (!text) {
      LOG_ERROR("Failed to generate debug text for UI: %", TTF_GetError());
      return;
    }

    float width = kLineHeight * text->w / text->h * context->window_height / context->window_width;
    DrawImage(text, kLineXOffset, kLineYOffset + kLineSpacing * i, width, kLineHeight);

    SDL_FreeSurface(text);
  }

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glDisableVertexAttribArray(0);
}

void UI::DrawImage(SDL_Surface* surface, float x, float y, float w, float h) {
  TextureManager::GetInstance()->BindStreamingTexture(texture_, GL_TEXTURE0);

  // We can try to shove the surface's underlying buffer straight to GPU, but
  // there are 3 potential problems with that:
  // 1.) The pitch may not = width * 4, even though it seems to always be true for SDL-created RGBA8888 surfaces.
  // 2.) Unlike OpenGL, in SDL endianness DOES affect storage order, because pixels are retrieved as u32,
  //     before the channel masks are applied and channels extracted.
  // 3.) The pixel format returned by TTF_RenderText_Blended() may change. As of this writing it's
  //     ARGB8888 (actually stored as BGRA on little-endian).
  // So we have a copy-free path for a few common cases (doing the swizzling in shader), and a fallback with loops.
  if (surface->pitch == (4 * surface->w) && surface->format->format == SDL_PIXELFORMAT_ARGB8888) { 
    TextureManager::GetInstance()->StreamData(reinterpret_cast<uint8_t*>(surface->pixels), surface->w, surface->h);
    if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
      shader_->SetUniform("texture_byte_order"_name, 1); // BGRA
    } else {
      shader_->SetUniform("texture_byte_order"_name, 2); // ARGB
    }
  } else {
    std::vector<uint8_t> data;
    data.reserve(surface->w * surface->h * 4);
    for (int row = 0; row < surface->h; ++row) {
      uint8_t* read_row_start = reinterpret_cast<uint8_t*>(surface->pixels) + row * surface->pitch;
      for (int col = 0; col < surface->w; ++col) {
        uint32_t pixel;
        // Only way to do safe type-punning! (the compiler will optimize it out)
        // This reads the whole pixel in machine byte order.
        std::memcpy(&pixel, read_row_start + col * 4, 4);

        // Now we can finally read out the channels.
        uint8_t r, g, b, a;
        SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
        data.push_back(r);
        data.push_back(g);
        data.push_back(b);
        data.push_back(a);
      }
    }
    TextureManager::GetInstance()->StreamData(data.data(), surface->w, surface->h);
    shader_->SetUniform("texture_byte_order"_name, 0); // RGBA
  }

  shader_->SetUniform("base_texture"_name, 0);
  shader_->SetUniform("xywh", glm::vec4(x, y, w, h));
  shader_->SetUniform("tex_xywh", glm::vec4(
      0.0f, 0.0f, static_cast<float>(surface->w) / kGpuTextureWidth, static_cast<float>(surface->h) / kGpuTextureHeight));

  UseVBO(GL_ARRAY_BUFFER, 0, GL_FLOAT, 2, vertices_vbo_id_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id_);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*) 0);
}
