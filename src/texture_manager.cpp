#include "texture_manager.h"

#include "lodepng/lodepng.h"

#include "utils.h"

namespace {
static constexpr const char* kTexturePathPrefix = "assets/art/textures/";
}

void TextureManager::BindTexture(const std::string& texture_name, GLenum texture_unit) {
  glActiveTexture(texture_unit);
  auto it = texture_cache_.find(texture_name);
  if (it == texture_cache_.end()) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    CHECK_GL_ERROR;
    glBindTexture(GL_TEXTURE_2D, texture_id);
    CHECK_GL_ERROR;

    texture_cache_.insert(std::make_pair(texture_name, texture_id));

    std::string png_path = std::string(kTexturePathPrefix) + texture_name + ".png";
    std::vector<uint8_t> image_data;
    uint32_t width, height;
    uint32_t error = lodepng::decode(image_data, width, height, png_path.c_str());

    if (error) {
      LOG_ERROR("Failed to load texture file %: %", texture_name, lodepng_error_text(error));
      return;
    }

    glTexImage2D(GL_TEXTURE_2D, /*level=*/0, /*internalFormat=*/GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data.data());
    CHECK_GL_ERROR;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 12); // Our largest textures are 2^12

    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    glBindTexture(GL_TEXTURE_2D, it->second);
  }
}

void TextureManager::BindTexture(GLuint texture, GLenum texture_unit) {
  glActiveTexture(texture_unit);
  glBindTexture(GL_TEXTURE_2D, texture);
}

GLuint TextureManager::MakeStreamingTexture(int width, int height) {
  glActiveTexture(GL_TEXTURE0);
  GLuint texture_id;
  glGenTextures(1, &texture_id);
  CHECK_GL_ERROR;
  glBindTexture(GL_TEXTURE_2D, texture_id);
  CHECK_GL_ERROR;
  glTexImage2D(GL_TEXTURE_2D, /*level=*/0, /*internalFormat=*/GL_RGBA, width, height,
               0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  CHECK_GL_ERROR;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return texture_id;
}

GLuint TextureManager::MakeDepthTexture(int width, int height) {
  glActiveTexture(GL_TEXTURE0);
  GLuint texture_id;
  glGenTextures(1, &texture_id);
  CHECK_GL_ERROR;
  glBindTexture(GL_TEXTURE_2D, texture_id);
  CHECK_GL_ERROR;
  glTexImage2D(GL_TEXTURE_2D, /*level=*/0, /*internalFormat=*/GL_DEPTH_COMPONENT32F, width, height,
               0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  CHECK_GL_ERROR;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return texture_id;
}

void TextureManager::StreamData(uint8_t* data, int width, int height) {
  glTexSubImage2D(GL_TEXTURE_2D, /*level=*/0, /*xoffset=*/0, /*yoffset=*/0, width, height,
                  GL_RGBA, GL_UNSIGNED_BYTE, data);
}
