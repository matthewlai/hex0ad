#include "texture_manager.h"

#include "lodepng/lodepng.h"

#include "utils.h"

namespace {
static constexpr const char* kTexturePathPrefix = "assets/art/textures/skins/";
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

    // These settings apply to the active texture unit, so we don't actually need to
    // do it for every texture loaded, but this is an easy way to ensure that we are applying
    // them to all the texture units we use, and the performance hit is negligible.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 12); // Our largest textures are 2^12

    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    glBindTexture(GL_TEXTURE_2D, it->second);
  }
}
