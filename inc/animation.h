#ifndef ANIMATION_H
#define ANIMATION_H

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

#include "animation_generated.h"
#include "utils.h"

class AnimationTemplate;

class Animation {
 public:
  Animation(const AnimationTemplate* animation_template, float speed)
      : template_(animation_template), done_(false), speed_(speed) {}

  void Start(uint64_t time_us) { start_time_us_ = time_us; }
  
  // Updates animation and returns new bone states.
  std::vector<glm::mat4> Update(uint64_t time_us);

  bool Done() { return done_; }

  Animation(const Animation& other) = delete;
  Animation(Animation&&) = default;

  virtual ~Animation() {}

 private:
  const AnimationTemplate* template_;

  // We don't implement repeated animations here because we want Actor to restart
  // animations, since that gives the actor a chance to pick a different one when multiple
  // are available (eg. for idle animations).
  bool done_;

  float speed_;

  uint64_t start_time_us_;
};

// Corresponds to an animation .fbs file, which corresponds to an animation DAE.
class AnimationTemplate {
 public:
  static AnimationTemplate& GetTemplate(const std::string& animation_path);

  std::unique_ptr<Animation> MakeAnimation(float speed) const { return std::make_unique<Animation>(this, speed); }

  std::string Name() const { return animation_data_->path()->str(); }

  // Duration of the animation in seconds.
  float Duration() const { return animation_data_->frame_time() * animation_data_->num_frames(); }

  std::vector<glm::mat4> GetFrame(float normalised_time) const;

 private:
  AnimationTemplate(const std::string& animation_path);

  std::vector<uint8_t> animation_raw_buffer_;
  const data::Animation* animation_data_;
};

#endif // ANIMATION_H
