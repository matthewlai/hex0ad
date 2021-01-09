#include "actor.h"

#include <fstream>
#include <random>
#include <stdexcept>
#include <vector>

#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"

#include "logger.h"
#include "renderer.h"
#include "shaders.h"
#include "utils.h"

namespace {
static constexpr const char* kActorPathPrefix = "assets/art/actors/";
static constexpr const char* kMeshPathPrefix = "assets/art/meshes/";

// Data about a mesh that has been uploaded to the GPU (used at least once).
struct MeshGPUData {
  ShaderProgram* shader;
  ShaderProgram* shadow_shader;

  GLuint vao_id;
  GLuint shadow_vao_id;

  GLsizei num_indices;
};

std::map<std::string, glm::mat4> GetAttachPoints(const std::string& mesh_file_name) {
  static std::map<std::string, std::map<std::string, glm::mat4>> cache;
  auto it = cache.find(mesh_file_name);
  if (it == cache.end()) {
    // This raw buffer only needs to survive for as long as we want to read
    // from the flat buffer. It will be deallocated when it goes out of scope
    // (once we have all the data we care about uploaded to the GPU).
    std::vector<std::uint8_t> raw_buffer =
        ReadWholeFile(std::string(kMeshPathPrefix) + mesh_file_name);
    const data::Mesh* mesh_data = data::GetMesh(raw_buffer.data());

    std::map<std::string, glm::mat4> ret;

    for (std::size_t i = 0; i < mesh_data->attachment_point_names()->size(); ++i) {
      float mtx[16];
      for (int j = 0; j < 16; ++j) {
        mtx[j] = mesh_data->attachment_point_transforms()->Get(i * 16 + j);
      }
      ret[mesh_data->attachment_point_names()->Get(i)->str()] = glm::make_mat4(mtx);
      LOG_DEBUG("Attachment Point %\n%", mesh_data->attachment_point_names()->Get(i)->str(),
                glm::to_string(glm::make_mat4(mtx)));
    }

    it = cache.insert(std::make_pair(mesh_file_name, ret)).first;
  }
  return it->second;
}

void RenderMesh(const std::string& mesh_file_name, const TextureSet& textures, const glm::mat4& vp,
                const glm::mat4& model, bool use_player_colour, Renderable::RenderContext* context) {
  static std::map<std::string, MeshGPUData> mesh_gpu_data_cache;
  bool shadow_pass = context->pass == RenderPass::kShadow;
  auto it = mesh_gpu_data_cache.find(mesh_file_name);
  if (it == mesh_gpu_data_cache.end()) {
    // This raw buffer only needs to survive for as long as we want to read
    // from the flat buffer. It will be deallocated when it goes out of scope
    // (once we have all the data we care about uploaded to the GPU).
    std::vector<std::uint8_t> raw_buffer =
        ReadWholeFile(std::string(kMeshPathPrefix) + mesh_file_name);
    const data::Mesh* mesh_data = data::GetMesh(raw_buffer.data());
    MeshGPUData data;
    data.shadow_shader = GetShader("shaders/shadow.vs", "shaders/shadow.fs");
    data.shader = GetShader("shaders/actor.vs", "shaders/actor.fs");
    data.shader->Activate();

    // Upload all the vertex attributes to the GPU.
    data.vao_id = Renderer::MakeVAO({
      Renderer::VBOSpec(*mesh_data->vertices(), 0, GL_FLOAT, 3),
      Renderer::VBOSpec(*mesh_data->normals(), 1, GL_FLOAT, 3),
      Renderer::VBOSpec(*mesh_data->tangents(), 2, GL_FLOAT, 3),
      Renderer::VBOSpec(*mesh_data->tex_coords(), 3, GL_FLOAT, 2),
      Renderer::VBOSpec(*mesh_data->ao_tex_coords(), 4, GL_FLOAT, 2),
    },
    Renderer::EBOSpec(*mesh_data->vertex_indices()));

    data.shadow_vao_id = Renderer::MakeVAO({
      Renderer::VBOSpec(*mesh_data->vertices(), 0, GL_FLOAT, 3),
    },
    Renderer::EBOSpec(*mesh_data->vertex_indices()));

    data.num_indices = mesh_data->vertex_indices()->size();
    it = mesh_gpu_data_cache.insert(std::make_pair(mesh_file_name, data)).first;
  }

  const MeshGPUData& data = it->second;

  ShaderProgram* shader = shadow_pass ? data.shadow_shader : data.shader;
  shader->Activate();

  glm::mat4 mvp = vp * model;

  shader->SetUniform("mvp"_name, mvp);

  Renderable::SetLightParams(context, shader);

  if (shadow_pass) {
    Renderer::UseVAO(data.shadow_vao_id);
    glDrawElements(GL_TRIANGLES, data.num_indices, GL_UNSIGNED_INT, (const void*) 0);
  } else {
    shader->SetUniform("model"_name, model);

    glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(model));
    shader->SetUniform("normal_matrix"_name, normal_matrix);

    glm::vec3 player_colour(0.0f, 0.0f, 1.0f);
    shader->SetUniform("player_colour"_name, player_colour);

    // Graphics settings.
    #define GraphicsSetting(upper, lower, type, default, toggle_key) \
      shader->SetUniform(NameLiteral(#lower), context->lower);
    GRAPHICS_SETTINGS
    #undef GraphicsSetting

    // Texture unit 0 for the base texture.
    if (textures.base_texture.empty()) {
      LOG_ERROR("No base texture. Skipping mesh.");
      return;
    }

    if (!use_player_colour) {
      shader->SetUniform("use_player_colour", 0);
    }

    TextureManager::GetInstance()->UseTextureSet(shader, textures);

    Renderer::UseVAO(data.vao_id);
    glDrawElements(GL_TRIANGLES, data.num_indices, GL_UNSIGNED_INT, (const void*) 0);
  }
}
}

/*static*/ ActorTemplate& ActorTemplate::GetTemplate(const std::string& actor_path) {
  static std::map<std::string, ActorTemplate> template_cache;
  static std::mt19937 rng(RngSeed());
  auto it = template_cache.find(actor_path);
  if (it == template_cache.end()) {
    it = template_cache.insert(std::make_pair(actor_path, ActorTemplate(actor_path, &rng))).first;
  }
  return it->second;
}

Actor::Actor(ActorTemplate* actor_template, bool randomize) : template_(actor_template), position_(0.0f, 0.0f, 0.0f), scale_(1.0f) {
  for (int group = 0; group < template_->NumGroups(); ++group) {
    std::vector<float> probability_densities;
    for (int variant = 0; variant < template_->NumVariants(group); ++variant) {
      probability_densities.push_back(template_->VariantFrequency(group, variant));
    }

    std::discrete_distribution dist(probability_densities.begin(), probability_densities.end());

    GroupConfig group_config;
    group_config.variant_selection = randomize ? dist(actor_template->Rng()) : 0;
    actor_config_.push_back(std::move(group_config));
  }
}

void Actor::Render(RenderContext* context) {
  // Models are supposed to be using 2m units, so scaling by 0.5 here give us 1m units to match rest of the game.
  // https://trac.wildfiregames.com/wiki/ArtScaleAndProportions
  Render(context,
         glm::translate(glm::mat4(1.0f), -position_) * glm::scale(glm::vec3(scale_, scale_, scale_)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)));
}

void Actor::Render(RenderContext* context, const glm::mat4& model) {
  if (context->pass == RenderPass::kGeometry || context->pass == RenderPass::kShadow) {
    template_->Render(context, actor_config_, model);
  }
}

ActorTemplate::ActorTemplate(const std::string& actor_path, std::mt19937* rng)
    : rng_(rng) {
  std::string full_path = std::string(kActorPathPrefix) + actor_path;
  actor_raw_buffer_ = ReadWholeFile(full_path);
  actor_data_ = data::GetActor(actor_raw_buffer_.data());
  LOG_INFO("Actor loaded: %", actor_data_->path()->str());
}

void ActorTemplate::Render(Renderable::RenderContext* context, const Actor::ActorConfig& config, const glm::mat4& model) {
  std::string mesh_path;
  TextureSet textures;
  std::map<std::string, std::vector<ActorTemplate*>> props;
  std::map<std::string, glm::mat4> attachpoints;

  bool shadow_pass = context->pass == RenderPass::kShadow;

  for (std::size_t group = 0; group < config.size(); ++group) {
    const data::Variant* variant = actor_data_->groups()->Get(group)->variants()->Get(config[group].variant_selection);

    if (variant->mesh_path() && !variant->mesh_path()->str().empty()) {
      mesh_path = variant->mesh_path()->str();
      attachpoints = GetAttachPoints(mesh_path);
    }

    if (!shadow_pass) {
      textures = TextureManager::GetInstance()->LoadTextures(*variant->textures(), textures);
    }

    for (const auto* prop : *variant->props()) {
      std::string attachpoint = prop->attachpoint()->str();
      std::string actor = prop->actor()->str();
      if (actor.empty()) {
        // We need to clear everything currently attached.
        props.erase(attachpoint);
      } else {
        props[attachpoint].push_back(&GetTemplate(actor));
      }
    }
  }

  if (mesh_path.empty()) {
    return;
  }

  attachpoints["root"] = glm::mat4(1.0f);

  auto* material_field = actor_data_->material();

  bool use_player_colour = true;

  if (!material_field) {
    LOG_ERROR("% has no material", actor_data_->path()->str());
  } else {
    std::string material = actor_data_->material()->str();
    use_player_colour = material.find("player") != std::string::npos;
  }

  RenderMesh(mesh_path, textures, context->projection * context->view,
             model * attachpoints["main_mesh"], use_player_colour, context);

  for (auto& [point, actor_templates] : props) {
    for (auto& actor_template : actor_templates) {
      auto it = attachpoints.find(point);

      if (it != attachpoints.end()) {
        glm::mat4 prop_model = model * it->second;
        // TODO: actually make ActorConfig recursive.
        Actor actor(actor_template, /*randomize=*/false);
        actor.Render(context, prop_model);
      }
    }
  }
}
