#ifndef ACTOR_H
#define ACTOR_H

#include <cstdint>
#include <optional>
#include <random>
#include <string>
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
  void Render(RenderContext* context) override;
  virtual ~Actor() {}

 private:
  Actor(ActorTemplate* actor_template);

  ActorTemplate* template_;
  std::vector<int> variant_selections_;

  friend class ActorTemplate;
};

// Corresponds to an actor .fbs file, which corresponds to an actor XML.
class ActorTemplate {
 public:
  ActorTemplate(const std::string& actor_path);

  Actor MakeActor() { return Actor(this); }

  int NumGroups() const { return actor_data_->groups()->size(); }
  int NumVariants(int group) const { return actor_data_->groups()->Get(group)->variants()->size(); }

  void Render(const std::vector<int>& variant_selections, Renderable::RenderContext* context);

  std::mt19937& Rng() {
    if (!rng_) {
      std::random_device rd;
      rng_ = std::mt19937(rd());
    }
    return *rng_;
  }

 private:
  std::optional<std::mt19937> rng_;
  std::vector<uint8_t> actor_raw_buffer_;
  const data::Actor* actor_data_;
};

#endif // ACTOR_H
