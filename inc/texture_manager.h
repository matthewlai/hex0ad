#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <map>
#include <string>

#include "platform_includes.h"

struct TextureSet {
  std::string base_texture;
  std::string norm_texture;
  std::string spec_texture;
  std::string ao_texture;
};

// This class manages all texture-related operations including activating
// texture units, loading and uploading textures to the GPU when necessary
// (on first use), and binding textures.
class TextureManager {
 public:
  static TextureManager* GetInstance() { 
  	static TextureManager instance;
  	return &instance;
  }

  void BindTexture(const std::string& texture_name, GLenum texture_unit);

 private:
  TextureManager() {}

  // Texture IDs for textures already uploaded to the GPU.
  std::map<std::string, GLuint> texture_cache_;
};

#endif // TEXTURE_MANAGER_H
