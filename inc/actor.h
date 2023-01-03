#ifndef ACTOR_H
#define ACTOR_H

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "animation.h"
#include "renderer.h"
#include "texture_manager.h"

#include "actor_generated.h"
#include "mesh_generated.h"

class ActorTemplate;

// An actor is a logical instantiation of an ActorTemplate, with sampled
// variant selections and state in world. The template must outlive any
// actor instantiated from it.
class Actor : public Renderable {
 public:
  enum class ActorState {
    kIdle,
  };

  Actor(const ActorTemplate* actor_template);

  void Update();

  void Render(RenderContext* context) override;
  void Render(RenderContext* context, const glm::mat4& model);

  void SetPosition(const glm::vec3& new_position) { position_ = new_position; }
  void SetScale(float new_scale) { scale_ = new_scale; }

  int NumGroups() const { return variant_selections_.size(); }
  int VariantSelection(int group) const { return variant_selections_[group]; }

  void AddPropIfNotExist(const std::string& attachpoint, const ActorTemplate& actor_template);

  void ClearAttachPoint(const std::string& attachpoint) {
    props_[attachpoint].clear();
  }

  std::map<std::string, std::vector<std::unique_ptr<Actor>>>* Props() {
    return &props_;
  }

  const std::vector<glm::mat4>& BoneTransforms() const { return bone_transforms_; }

  Actor(const Actor& other) = delete;
  Actor(Actor&& actor) = default;

  virtual ~Actor() {}

 private:
  ActorState state_;

  std::unique_ptr<Animation> active_animation_;

  // AnimationSpecs for the current variant selections.
  std::map<std::string, std::vector<const data::AnimationSpec*>> animation_specs_;

  // Variant selection for each group.
  std::vector<int> variant_selections_;

  std::map<std::string, std::vector<std::unique_ptr<Actor>>> props_;

  const ActorTemplate* template_;

  glm::vec3 position_;
  float scale_;

  std::vector<glm::mat4> bone_transforms_;
};

// Corresponds to an actor .fbs file, which corresponds to an actor XML.
class ActorTemplate {
 public:
  static ActorTemplate& GetTemplate(const std::string& actor_path);

  Actor MakeActor() { return Actor(this); }

  std::string Name() const { return actor_data_->path()->str(); }

  int NumGroups() const { return actor_data_->groups()->size(); }
  int NumVariants(int group) const { return actor_data_->groups()->Get(group)->variants()->size(); }
  float VariantFrequency(int group, int variant) const {
    return actor_data_->groups()->Get(group)->variants()->Get(variant)->frequency();
  }

  // Render a variant from a group. Props are ignored.
  void Render(Renderable::RenderContext* context, Actor* actor, const glm::mat4& model) const;

  // Get all the animation paths with the actor's current selection of variants.
  std::map<std::string, std::vector<const data::AnimationSpec*>> AnimationSpecs(const Actor* actor) const;

  // Get all the joint bind pose inverses with the actor's current selection of variants.
  std::vector<glm::mat4> BindPoseInverses(const Actor* actor) const;

  std::mt19937& Rng() const { return *rng_; }

 private:
  ActorTemplate(const std::string& actor_path, std::mt19937* rng);

  mutable std::mt19937* rng_;
  std::vector<uint8_t> actor_raw_buffer_;
  const data::Actor* actor_data_;
};

#endif // ACTOR_H
