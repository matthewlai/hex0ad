#include "actor.h"

#include <fstream>
#include <random>
#include <stdexcept>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"
#pragma GCC diagnostic pop

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
  bool skinned;
};

struct AttachmentPoints {
  // Either relative to root or bone.
  glm::mat4 transform;

  // Relative to bone if bone != 0xFF.
  uint8_t bone;
};

std::map<std::string, AttachmentPoints> GetAttachPoints(const std::string& mesh_file_name) {
  static std::map<std::string, std::map<std::string, AttachmentPoints>> cache;
  auto it = cache.find(mesh_file_name);
  if (it == cache.end()) {
    // This raw buffer only needs to survive for as long as we want to read
    // from the flat buffer. It will be deallocated when it goes out of scope
    // (once we have all the data we care about uploaded to the GPU).
    std::vector<std::uint8_t> raw_buffer =
        ReadWholeFile(std::string(kMeshPathPrefix) + mesh_file_name);
    const data::Mesh* mesh_data = data::GetMesh(raw_buffer.data());

    std::map<std::string, AttachmentPoints> ret;

    for (std::size_t i = 0; i < mesh_data->attachment_point_names()->size(); ++i) {
      float mtx[16];
      for (int j = 0; j < 16; ++j) {
        mtx[j] = mesh_data->attachment_point_transforms()->Get(i * 16 + j);
      }
      AttachmentPoints pt;
      pt.transform = glm::make_mat4(mtx);
      pt.bone = mesh_data->attachment_point_bones()->Get(i);
      ret[mesh_data->attachment_point_names()->Get(i)->str()] = std::move(pt);
      LOG_DEBUG("Attachment Point % (bone: %)\n%", mesh_data->attachment_point_names()->Get(i)->str(),
                pt.bone, glm::to_string(pt.transform));
    }

    it = cache.insert(std::make_pair(mesh_file_name, ret)).first;
  }
  return it->second;
}

void RenderMesh(const std::string& mesh_file_name, const TextureSet& textures, const glm::mat4& vp,
                const glm::mat4& model, std::optional<glm::vec3> maybe_alpha_colour,
                const std::vector<glm::mat4>& bone_transforms, Renderable::RenderContext* context) {
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

    data.skinned = mesh_data->bind_pose_transforms()->size() > 0;

    // Upload all the vertex attributes to the GPU.
    data.vao_id = Renderer::MakeVAO({
      Renderer::VBOSpec(*mesh_data->vertices(), 0, GL_FLOAT, 3),
      Renderer::VBOSpec(*mesh_data->normals(), 1, GL_FLOAT, 3),
      Renderer::VBOSpec(*mesh_data->tangents(), 2, GL_FLOAT, 3),
      Renderer::VBOSpec(*mesh_data->tex_coords(), 3, GL_FLOAT, 2),
      Renderer::VBOSpec(*mesh_data->ao_tex_coords(), 4, GL_FLOAT, 2),
      Renderer::VBOSpec(*mesh_data->bone_indices(), 5, GL_BYTE, 4),
      Renderer::VBOSpec(*mesh_data->bone_weights(), 6, GL_FLOAT, 4),
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

    // Graphics settings.
    #define GraphicsSetting(upper, lower, type, default, toggle_key) \
      shader->SetUniform(NameLiteral(#lower), context->lower);
    GRAPHICS_SETTINGS
    #undef GraphicsSetting

    if (maybe_alpha_colour) {
      shader->SetUniform("alpha_colour"_name, *maybe_alpha_colour);
      shader->SetUniform("use_alpha_colour"_name, 1);
    } else {
      shader->SetUniform("use_alpha_colour"_name, 0);
    }

    // Texture unit 0 for the base texture.
    if (textures.base_texture.empty()) {
      LOG_ERROR("No base texture. Skipping mesh.");
      return;
    }

    shader->SetUniform("skinning"_name, data.skinned ? 1 : 0);

    if (data.skinned) {
      shader->SetUniform("bone_transforms"_name, bone_transforms);
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

Actor::Actor(const ActorTemplate* actor_template)
    : state_(ActorState::kIdle), template_(actor_template), position_(0.0f, 0.0f, 0.0f), rotation_rad_(0.0f), scale_(1.0f) {
  for (int group = 0; group < template_->NumGroups(); ++group) {
    std::vector<float> probability_densities;
    for (int variant = 0; variant < template_->NumVariants(group); ++variant) {
      probability_densities.push_back(template_->VariantFrequency(group, variant));
    }

    std::discrete_distribution dist(probability_densities.begin(), probability_densities.end());

    variant_selections_.push_back(dist(actor_template->Rng()));
  }

  animation_specs_ = template_->AnimationSpecs(this);
}

void Actor::Update(uint64_t time_us) {
  if (!active_animation_ || active_animation_->Done()) {
    // We are out of animation. See if we can start a new one.
    if (state_ == ActorState::kIdle) {
      if (animation_specs_.find("Idle") != animation_specs_.end()) {
        const auto& candidates = animation_specs_["Idle"];
        const auto& spec = candidates[0];
        const auto& animation_template = AnimationTemplate::GetTemplate(spec->path()->str());
        active_animation_ = animation_template.MakeAnimation(spec->speed());
        active_animation_->Start(time_us);
        LOG_INFO("Starting new animation: %", candidates[0]->path()->str());
      }
    }
  }
  if (active_animation_) {
    bone_transforms_ = active_animation_->Update(time_us);
  }

  for (auto& [point, props] : props_) {
    for (auto& prop : props) {
      prop->Update(time_us);
    }
  }
}

void Actor::Render(RenderContext* context) {
  // Models are supposed to be using 2m units, so scaling by 0.5 here give us 1m units to match rest of the game.
  // https://trac.wildfiregames.com/wiki/ArtScaleAndProportions
  Render(context,
         glm::translate(glm::mat4(1.0f), -position_) * glm::rotate(rotation_rad_, glm::vec3(0.0f, 0.0f, 1.0f)) *
         glm::scale(glm::vec3(scale_ * 0.5f, scale_ * 0.5f, scale_ * 0.5f)));
}

void Actor::Render(RenderContext* context, const glm::mat4& model) {
  if (context->pass == RenderPass::kGeometry || context->pass == RenderPass::kShadow) {
    template_->Render(context, this, model);
  }
}

void Actor::AddPropIfNotExist(const std::string& attachpoint, const ActorTemplate& actor_template) {
  for (const auto& prop : props_[attachpoint]) {
    if (prop->template_->Name() == actor_template.Name()) {
      return;
    }
  }
  props_[attachpoint].push_back(std::unique_ptr<Actor>(new Actor(&actor_template)));
}

ActorTemplate::ActorTemplate(const std::string& actor_path, std::mt19937* rng)
    : rng_(rng) {
  std::string full_path = std::string(kActorPathPrefix) + actor_path + ".fb";
  actor_raw_buffer_ = ReadWholeFile(full_path);
  actor_data_ = data::GetActor(actor_raw_buffer_.data());
  LOG_INFO("Actor loaded: %", actor_data_->path()->str());
}

void ActorTemplate::Render(Renderable::RenderContext* context, Actor* actor, const glm::mat4& model) const {
  std::string mesh_path;
  TextureSet textures;
  std::map<std::string, std::vector<ActorTemplate*>> props;
  std::map<std::string, AttachmentPoints> attachpoints;
  std::optional<glm::vec3> object_colour;

  bool shadow_pass = context->pass == RenderPass::kShadow;

  for (int group = 0; group < actor->NumGroups(); ++group) {
    const data::Variant* variant = actor_data_->groups()->Get(group)->variants()->Get(actor->VariantSelection(group));

    if (variant->mesh_path() && !variant->mesh_path()->str().empty()) {
      mesh_path = variant->mesh_path()->str();
      attachpoints = GetAttachPoints(mesh_path);
    }

    if (!shadow_pass) {
      textures = TextureManager::GetInstance()->LoadTextures(*variant->textures(), textures);
    }

    for (const auto* prop : *variant->props()) {
      std::string attachpoint = prop->attachpoint()->str();
      std::string prop_actor = prop->actor()->str();
      if (prop_actor.empty()) {
        // We need to clear everything currently attached.
        actor->ClearAttachPoint(attachpoint);
      } else {
        actor->AddPropIfNotExist(attachpoint, GetTemplate(prop_actor));
      }
    }

    if (variant->object_colour()) {
      object_colour = glm::vec3(
          variant->object_colour()->r(), variant->object_colour()->g(), variant->object_colour()->b());
    }
  }

  if (mesh_path.empty()) {
    return;
  }

  auto* material_field = actor_data_->material();

  std::optional<glm::vec3> maybe_alpha_colour;

  if (!material_field) {
    LOG_ERROR("% has no material", actor_data_->path()->str());
  } else {
    std::string material = actor_data_->material()->str();
    if (object_colour) {
      maybe_alpha_colour = *object_colour;
    } else if (material.find("player") != std::string::npos) {
      maybe_alpha_colour = glm::vec3(0.6f, 0.0f, 0.0f);
    }
  }

  bool skinning = !actor->BoneTransforms().empty();

  // Make bone transforms pre-multiplied by bind pose inverses, and
  // with the virtual bind pose bone added.
  std::vector<glm::mat4> final_bone_transforms;

  if (skinning) {
    auto to_bone_space = BindPoseInverses(actor);

    if (to_bone_space.size() != actor->BoneTransforms().size()) {
      LOG_ERROR("Bind pose inverse and animation frame joint count mismatch: % != %",
                to_bone_space.size(), actor->BoneTransforms().size());
      return;
    }

    final_bone_transforms.resize(to_bone_space.size() + 1);

    for (std::size_t joint = 0; joint != to_bone_space.size(); ++joint) {
      final_bone_transforms[joint] = actor->BoneTransforms()[joint] * to_bone_space[joint];
    }

    // Special bind pose virtual bone (identity in our case, since we pre-applied model
    // transform in the model)
    final_bone_transforms[final_bone_transforms.size() - 1] = glm::mat4(1.0f);
  }

  // If we are skinning, we should render to "root" because our inverse bind
  // pose transform already takes that into account. Otherwise we use the
  // "mesh_root" point which is "root" + root entity transform.
  glm::mat4 render_root = skinning ?
      attachpoints["root"].transform : attachpoints["mesh_root"].transform;

  RenderMesh(mesh_path, textures, context->projection * context->view,
             model * render_root, maybe_alpha_colour,
             final_bone_transforms, context);

  for (auto& [point, prop_actors] : *(actor->Props())) {
    auto it = attachpoints.find(point);
    if (it != attachpoints.end()) {
      const AttachmentPoints& pt = it->second;
      for (auto& prop_actor : prop_actors) {
        glm::mat4 prop_model;
        if (pt.bone == 0xFF || pt.bone >= actor->BoneTransforms().size()) {
          prop_model = model * attachpoints["root"].transform * pt.transform;
        } else {
          prop_model = model * attachpoints["root"].transform * actor->BoneTransforms()[pt.bone] * pt.transform;
        }
        prop_actor->Render(context, prop_model);
      }
    }
  }
}

std::map<std::string, std::vector<const data::AnimationSpec*>> ActorTemplate::AnimationSpecs(
    const Actor* actor) const {
  std::map<std::string, std::vector<const data::AnimationSpec*>> ret;
  for (int group = 0; group < actor->NumGroups(); ++group) {
    const data::Variant* variant = actor_data_->groups()->Get(group)->variants()->Get(actor->VariantSelection(group));
    for (const auto* animation_spec : *(variant->animations())) {
      std::string name = animation_spec->name()->str();
      auto it = ret.find(name);
      if (it == ret.end()) {
        it = ret.insert(std::make_pair(name, std::vector<const data::AnimationSpec*>())).first;
      }
      it->second.push_back(animation_spec);
    }
  }
  return ret;
}

std::vector<glm::mat4> ActorTemplate::BindPoseInverses(const Actor* actor) const {
  std::string mesh_path;
  for (int group = 0; group < actor->NumGroups(); ++group) {
    const data::Variant* variant = actor_data_->groups()->Get(group)->variants()->Get(actor->VariantSelection(group));
    if (variant->mesh_path() && !variant->mesh_path()->str().empty()) {
      mesh_path = variant->mesh_path()->str();
      break;
    }
  }
  if (mesh_path.empty()) {
    return {};
  }
  static std::unordered_map<std::string, std::vector<glm::mat4>> cache;
  auto it = cache.find(mesh_path);
  if (it == cache.end()) {
    auto mesh_file_content = ReadWholeFile(std::string(kMeshPathPrefix) + mesh_path);
    const auto* mesh = data::GetMesh(mesh_file_content.data());
    std::size_t num_joints = mesh->bind_pose_transforms()->size() / 7;
    std::vector<glm::mat4> ret(num_joints);
    for (std::size_t joint = 0; joint < num_joints; ++joint) {
      const float* joint_ptr = mesh->bind_pose_transforms()->data() + 7 * joint;
      ret[joint] = ReadBoneTransform(joint_ptr).ToInvMatrix();
    }
    it = cache.insert({mesh_path, ret}).first;
  }  
  return it->second;
}
