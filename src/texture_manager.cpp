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

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // This is needed if we want to use hardware shadow sampler.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

  return texture_id;
}

void TextureManager::ResizeStreamingTexture(GLuint texture_id, int width, int height) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, /*level=*/0, /*internalFormat=*/GL_RGBA, width, height,
               0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  CHECK_GL_ERROR;
}

void TextureManager::ResizeDepthTexture(GLuint texture_id, int width, int height) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, /*level=*/0, /*internalFormat=*/GL_DEPTH_COMPONENT32F, width, height,
               0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  CHECK_GL_ERROR;
}

void TextureManager::StreamData(uint8_t* data, int width, int height) {
  glTexSubImage2D(GL_TEXTURE_2D, /*level=*/0, /*xoffset=*/0, /*yoffset=*/0, width, height,
                  GL_RGBA, GL_UNSIGNED_BYTE, data);
}

TextureSet TextureManager::LoadTextures(const flatbuffers::Vector<flatbuffers::Offset<data::Texture>>& textures,
                                        const TextureSet& existing) {
  TextureSet ret = existing;
  for (const auto* texture : textures) {
    auto texture_name = texture->name()->str();
    auto texture_file = texture->file()->str();
    if (texture_name == "baseTex") {
      LOG_DEBUG("baseTex found: %", texture_file);
      ret.base_texture = texture_file;
    } else if (texture_name == "normTex") {
      LOG_DEBUG("normTex found: %", texture_file);
      ret.norm_texture = texture_file;
    } else if (texture_name == "specTex") {
      LOG_DEBUG("specTex found: %", texture_file);
      ret.spec_texture = texture_file;
    } else if (texture_name == "aoTex") {
      LOG_DEBUG("aoTex found: %", texture_file);
      ret.ao_texture = texture_file;
    }
  }
  return ret;
}

void TextureManager::UseTextureSet(ShaderProgram* shader, const TextureSet& textures) {
  BindTexture(textures.base_texture, GL_TEXTURE0);
  shader->SetUniform("base_texture"_name, 0);

  if (!textures.spec_texture.empty()) {
    TextureManager::GetInstance()->BindTexture(textures.spec_texture, GL_TEXTURE1);
    shader->SetUniform("spec_texture"_name, 1);
  } else {
    shader->SetUniform("use_specular_highlight"_name, 0);
  }

  if (!textures.norm_texture.empty()) {
    TextureManager::GetInstance()->BindTexture(textures.norm_texture, GL_TEXTURE2);
    shader->SetUniform("norm_texture"_name, 2);
  } else {
    shader->SetUniform("use_normal_map"_name, 0);
  }

  if (!textures.ao_texture.empty()) {
    TextureManager::GetInstance()->BindTexture(textures.ao_texture, GL_TEXTURE3);
    shader->SetUniform("ao_texture"_name, 3);
  } else {
    shader->SetUniform("use_ao_map"_name, 0);
  }
}
