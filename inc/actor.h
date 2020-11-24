#ifndef ACTOR_H
#define ACTOR_H

#include <optional>
#include <random>
#include <string>
#include <vector>

class ActorTemplate;

// An actor is a logical instantiation of an ActorTemplate, with sampled
// variant selections and state in world. The template must outlive any
// actor instantiated from it.
class Actor {
 public:

 private:
  Actor(ActorTemplate* actor_template);

  ActorTemplate* template_;
  std::vector<int> variant_selections_;

  friend class ActorTemplate;
};

// Corresponds to an actor XML file (all groups and variants).
class ActorTemplate {
 public:
  struct Variant {
    // Variants with frequency > 0 may be randomly selected.
    // Variants with frequency = 0 are either disabled or selected by name.
    float frequency = 0.0f;
    std::string name;

    // Each variant may have animations, (1x) mesh, props, and/or textures.
    std::optional<std::string> mesh_path;
  };
  using Group = std::vector<Variant>;

  ActorTemplate(const std::string& actor_path);

  Actor MakeActor() { return Actor(this); }

  int NumGroups() const { return static_cast<int>(groups_.size()); }
  int NumVariants(int group) const { return static_cast<int>(groups_[group].size()); }

  std::mt19937& Rng() {
    if (!rng_) {
      std::random_device rd;
      rng_ = std::mt19937(rd());
    }
    return *rng_;
  }

 private:
  std::vector<Group> groups_;
  std::optional<std::mt19937> rng_;
};

#endif // ACTOR_H
