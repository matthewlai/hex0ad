#include "actor.h"

#include <fstream>
#include <stdexcept>
#include <vector>

#include "glm/gtc/matrix_inverse.hpp"
#include "lodepng/lodepng.h"

#include "logger.h"
#include "renderer.h"
#include "shaders.h"
#include "utils.h"

namespace {
static constexpr const char* kActorPathPrefix = "assets/art/actors/";
static constexpr const char* kMeshPathPrefix = "assets/art/meshes/";
static constexpr const char* kTexturePathPrefix = "assets/art/textures/skins/";

// Data about a mesh that has been uploaded to the GPU (used at least once).
struct MeshGPUData {
  ShaderProgram* shader;

  std::string base_texture;
  std::string norm_texture;
  std::string spec_texture;
  std::string ao_texture;

  GLint mvp_loc;
  GLint model_loc;
  GLint normal_matrix_loc;
  GLint base_tex_loc;
  GLint norm_tex_loc;
  GLint spec_tex_loc;
  GLint ao_tex_loc;

  GLint light_pos_loc;
  GLint player_colour_loc;
  GLint eye_pos_loc;

  #define GraphicsSetting(upper, lower, type, default, toggle_key) GLint lower ## _loc;
  GRAPHICS_SETTINGS
  #undef GraphicsSetting

  GLuint vertices_vbo_id;
  GLuint normals_vbo_id;
  GLuint tangents_vbo_id;
  GLuint tex_coords_vbo_id;
  GLuint ao_tex_coords_vbo_id;

  GLuint indices_vbo_id;
  GLsizei num_indices;
};

void BindTexture(const std::string& texture_name, GLenum texture_unit) {
  static std::map<std::string, GLuint> texture_cache;
  glActiveTexture(texture_unit);
  CHECK_GL_ERROR;
  auto it = texture_cache.find(texture_name);
  if (it == texture_cache.end()) {
    glActiveTexture(GL_TEXTURE31);
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    CHECK_GL_ERROR;
    glBindTexture(GL_TEXTURE_2D, texture_id);
    CHECK_GL_ERROR;

    texture_cache.insert(std::make_pair(texture_name, texture_id));

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 10); // Our largest textures are 2^10
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    glBindTexture(GL_TEXTURE_2D, it->second);
    CHECK_GL_ERROR;
  }
}

template <typename BufT>
GLuint MakeAndUploadVBO(GLenum binding_target, BufT* buf) {
  GLuint vbo_id;
  glGenBuffers(1, &vbo_id);
  glBindBuffer(binding_target, vbo_id);
  glBufferData(binding_target, sizeof(typename BufT::return_type) * buf->size(), 
               buf->data(), GL_STATIC_DRAW);
  return vbo_id;
}

void UseVBO(GLenum binding_target, int attrib_location, GLenum gl_type, int components_per_element, GLuint vbo_id) {
  glEnableVertexAttribArray(attrib_location);
  glBindBuffer(binding_target, vbo_id);
  glVertexAttribPointer(attrib_location, components_per_element, gl_type, GL_FALSE, 0, (const void*) 0);
}

void RenderMesh(const std::string& mesh_file_name, const glm::mat4& vp, const glm::mat4& model, Renderable::RenderContext* context) {
  static std::map<std::string, MeshGPUData> mesh_gpu_data_cache;
  auto it = mesh_gpu_data_cache.find(mesh_file_name);
  if (it == mesh_gpu_data_cache.end()) {
    // This raw buffer only needs to survive for as long as we want to read
    // from the flat buffer. It will be deallocated when it goes out of scope
    // (once we have all the data we care about uploaded to the GPU).
    std::vector<std::uint8_t> raw_buffer =
        ReadWholeFile(std::string(kMeshPathPrefix) + mesh_file_name);
    const data::Mesh* mesh_data = data::GetMesh(raw_buffer.data());
    MeshGPUData data;
    data.shader = GetShader("shaders/actor.vs", "shaders/actor.fs");
    data.shader->Activate();
    data.mvp_loc = data.shader->GetUniformLocation("mvp");
    data.normal_matrix_loc = data.shader->GetUniformLocation("normal_matrix");
    data.model_loc = data.shader->GetUniformLocation("model");
    data.player_colour_loc = data.shader->GetUniformLocation("player_colour");
    data.light_pos_loc = data.shader->GetUniformLocation("light_pos");
    data.eye_pos_loc = data.shader->GetUniformLocation("eye_pos");

    data.base_tex_loc = data.shader->GetUniformLocation("base_texture");
    data.norm_tex_loc = data.shader->GetUniformLocation("norm_texture");
    data.spec_tex_loc = data.shader->GetUniformLocation("spec_texture");
    data.ao_tex_loc = data.shader->GetUniformLocation("ao_texture");

    #define GraphicsSetting(upper, lower, type, default, toggle_key) \
      data.lower ## _loc = data.shader->GetUniformLocation(#lower);
    GRAPHICS_SETTINGS
    #undef GraphicsSetting

    data.base_texture = "";
    for (std::size_t i = 0; i < mesh_data->texture_types()->size(); ++i) {
      auto texture_type = mesh_data->texture_types()->Get(i)->str();
      auto texture_file = mesh_data->texture_files()->Get(i)->str();
      if (texture_type == "baseTex") {
        LOG_DEBUG("baseTex found: %", texture_file);
        data.base_texture = texture_file;
      } else if (texture_type == "normTex") {
        LOG_DEBUG("normTex found: %", texture_file);
        data.norm_texture = texture_file;
      } else if (texture_type == "specTex") {
        LOG_DEBUG("specTex found: %", texture_file);
        data.spec_texture = texture_file;
      } else if (texture_type == "aoTex") {
        LOG_DEBUG("aoTex found: %", texture_file);
        data.ao_texture = texture_file;
      }
    }

    // Upload all the vertex attributes to the GPU.
    data.vertices_vbo_id = MakeAndUploadVBO(GL_ARRAY_BUFFER, mesh_data->vertices());
    data.normals_vbo_id = MakeAndUploadVBO(GL_ARRAY_BUFFER, mesh_data->normals());
    data.tangents_vbo_id = MakeAndUploadVBO(GL_ARRAY_BUFFER, mesh_data->tangents());
    data.tex_coords_vbo_id = MakeAndUploadVBO(GL_ARRAY_BUFFER, mesh_data->tex_coords());
    data.ao_tex_coords_vbo_id = MakeAndUploadVBO(GL_ARRAY_BUFFER, mesh_data->ao_tex_coords());
    data.indices_vbo_id = MakeAndUploadVBO(GL_ELEMENT_ARRAY_BUFFER, mesh_data->vertex_indices());

    data.num_indices = mesh_data->vertex_indices()->size();
    it = mesh_gpu_data_cache.insert(std::make_pair(mesh_file_name, data)).first;
  }

  const MeshGPUData& data = it->second;

  data.shader->Activate();

  glm::mat4 mvp = vp * model;
  glUniformMatrix4fv(data.mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));

  glUniformMatrix4fv(data.model_loc, 1, GL_FALSE, glm::value_ptr(model));

  glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(model));
  glUniformMatrix3fv(data.normal_matrix_loc, 1, GL_FALSE, glm::value_ptr(normal_matrix));

  glm::vec3 player_colour(0.0f, 0.0f, 1.0f);
  glUniform3fv(data.player_colour_loc, 1, glm::value_ptr(player_colour));

  glUniform3fv(data.light_pos_loc, 1, glm::value_ptr(context->light_pos));

  glUniform3fv(data.eye_pos_loc, 1, glm::value_ptr(context->eye_pos));

  // Graphics settings.
  #define GraphicsSetting(upper, lower, type, default, toggle_key) \
    glUniform1i(data.lower ## _loc, context->lower);
  GRAPHICS_SETTINGS
  #undef GraphicsSetting

  // Texture unit 0 for the base texture.
  BindTexture(data.base_texture, GL_TEXTURE0);
  glUniform1i(data.base_tex_loc, 0);

  // Texture unit 1 for the normal texture.
  BindTexture(data.norm_texture, GL_TEXTURE1);
  glUniform1i(data.norm_tex_loc, 1);

  // Texture unit 2 for the spec texture
  BindTexture(data.spec_texture, GL_TEXTURE2);
  glUniform1i(data.spec_tex_loc, 2);

  // Texture unit 3 for the spec texture
  BindTexture(data.ao_texture, GL_TEXTURE3);
  glUniform1i(data.ao_tex_loc, 3);

  UseVBO(GL_ARRAY_BUFFER, 0, GL_FLOAT, 3, data.vertices_vbo_id);
  UseVBO(GL_ARRAY_BUFFER, 1, GL_FLOAT, 3, data.normals_vbo_id);
  UseVBO(GL_ARRAY_BUFFER, 2, GL_FLOAT, 3, data.tangents_vbo_id);
  UseVBO(GL_ARRAY_BUFFER, 3, GL_FLOAT, 2, data.tex_coords_vbo_id);
  UseVBO(GL_ARRAY_BUFFER, 4, GL_FLOAT, 2, data.ao_tex_coords_vbo_id);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.indices_vbo_id);
  glDrawElements(GL_TRIANGLES, data.num_indices, GL_UNSIGNED_INT, (const void*) 0);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glDisableVertexAttribArray(3);
  glDisableVertexAttribArray(4);
}
}

Actor::Actor(ActorTemplate* actor_template) : template_(actor_template) {
  for (int group = 0; group < template_->NumGroups(); ++group) {
    std::uniform_int_distribution<> dist(0, template_->NumVariants(group) - 1);
    variant_selections_.push_back(dist(actor_template->Rng()));
  }
}

void Actor::Render(RenderContext* context) {
  template_->Render(variant_selections_, context);
}

ActorTemplate::ActorTemplate(const std::string& actor_path) {
  std::string full_path = std::string(kActorPathPrefix) + actor_path;
  actor_raw_buffer_ = ReadWholeFile(full_path);
  actor_data_ = data::GetActor(actor_raw_buffer_.data());
  LOG_INFO("Actor loaded: %", actor_data_->path()->str());
}

void ActorTemplate::Render(const std::vector<int>& variant_selections, Renderable::RenderContext* context) {
  for (std::size_t group = 0; group < variant_selections.size(); ++group) {
    int selection = variant_selections[group];
    const data::Variant* variant = actor_data_->groups()->Get(group)->variants()->Get(selection);
    // Don't render if no mesh for now (we aren't dealing with props yet).
    if (!variant->mesh_path() || variant->mesh_path()->str().empty()) {
      continue;
    }
    std::string mesh_path = variant->mesh_path()->str();
    glm::mat4 model = glm::mat4(1.0f);
    RenderMesh(mesh_path, context->vp, model, context);
  }
}
