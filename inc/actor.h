#ifndef ACTOR_H
#define ACTOR_H

#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "renderer.h"

#include "actor_generated.h"
#include "mesh_generated.h"

class ActorTemplate;

// An actor is a logical instantiation of an ActorTemplate, with sampled
// variant selections and state in world. The template must outlive any
// actor instantiated from it.
class Actor : public Renderable {
 public:
  // This struct describes how a group should be rendered. That includes a variant
  // selection and potentially additional Actors (which have their own list of group
  // configs) in case the variant includes props (actors attached to the actor).
  struct GroupConfig {
    int variant_selection;
    std::vector<std::pair<std::string, Actor>> props;
  };

  using ActorConfig = std::vector<GroupConfig>;

  void Render(RenderContext* context) override;
  void Render(RenderContext* context, const glm::mat4& model);

  virtual ~Actor() {}

 private:
  Actor(ActorTemplate* actor_template, bool randomize = true);

  ActorTemplate* template_;
  ActorConfig actor_config_;

  friend class ActorTemplate;
};

// Corresponds to an actor .fbs file, which corresponds to an actor XML.
class ActorTemplate {
 public:
  static ActorTemplate& GetTemplate(const std::string& actor_path);

  Actor MakeActor() { return Actor(this); }

  std::string Name() const { return actor_data_->path()->str(); }

  int NumGroups() const { return actor_data_->groups()->size(); }
  int NumVariants(int group) const { return actor_data_->groups()->Get(group)->variants()->size(); }

  // Render a variant from a group. Props are ignored.
  void Render(Renderable::RenderContext* context, const Actor::ActorConfig& config, const glm::mat4& model);

  std::mt19937& Rng() { return *rng_; }

 private:
  ActorTemplate(const std::string& actor_path, std::mt19937* rng);

  std::mt19937* rng_;
  std::vector<uint8_t> actor_raw_buffer_;
  const data::Actor* actor_data_;
};

#endif // ACTOR_H
