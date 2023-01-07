#include "animation.h"

#include <cmath>
#include <fstream>
#include <random>
#include <stdexcept>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"

#include "logger.h"
#include "utils.h"

namespace {
static constexpr const char* kAnimationPathPrefix = "assets/art/animation/";
}

std::vector<glm::mat4> Animation::Update(uint64_t time_us) {
  float normalised_time = static_cast<float>((time_us - start_time_us_) / 1000000.0f) / template_->Duration() * speed_;
  if (normalised_time >= 1.0f) {
    done_ = true;
  }
  // Calling GetFrame with normalised_time > 1.0 is fine (clamped in GetFrame).
  return template_->GetFrame(normalised_time);
}

/*static*/ AnimationTemplate& AnimationTemplate::GetTemplate(const std::string& animation_path) {
  static std::map<std::string, AnimationTemplate> template_cache;
  auto it = template_cache.find(animation_path);
  if (it == template_cache.end()) {
    it = template_cache.insert(std::make_pair(animation_path, AnimationTemplate(animation_path))).first;
  }
  return it->second;
}

std::vector<glm::mat4> AnimationTemplate::GetFrame(float normalised_time) const {
  // TODO: Special case the interpolation to end at last frame for non-repeating animations.
  int num_frames = animation_data_->num_frames();

  /*
  Example: 2 frame animation, if normalised_time = 0.3
  T = 0     1     0
      |-----|-----
          ^
      Prev = 0
      Next = 1
  Normalised_time = 0.7
  T = 0     1     0
      |-----|-----
               ^
      Prev = 1
      Next = 0 (implied repeating animation)
  */

  normalised_time = std::clamp<float>(normalised_time, 0.0f, 1.0f);
  float frame_time = 1.0f / animation_data_->num_frames();

  int frame_number_prev = std::floor(normalised_time * num_frames);

  /* 0 = prev, 1 = next */
  float interp_arg = (normalised_time - frame_time * frame_number_prev) / frame_time;

  int frame_number_next = frame_number_prev + 1;

  // Handle some corner cases.
  if (frame_number_prev == num_frames) {
    frame_number_prev = 0;
    frame_number_next = 0;
    interp_arg = 0.0f;
  } else if (frame_number_prev == (num_frames - 1)) {
    frame_number_next = 0;
  }

  int stride = animation_data_->num_bones() * 7;
  const float* prev_frame_offset = animation_data_->bone_states()->data() + stride * frame_number_prev;
  const float* next_frame_offset = animation_data_->bone_states()->data() + stride * frame_number_next;
  std::vector<glm::mat4> ret(animation_data_->num_bones());

  for (std::size_t bone = 0; bone < animation_data_->num_bones(); ++bone) {
    BoneTransform prev_transform = ReadBoneTransform(prev_frame_offset + bone * 7);
    BoneTransform next_transform = ReadBoneTransform(next_frame_offset + bone * 7);
    BoneTransform interpolated;
    interpolated.translation = glm::mix(prev_transform.translation, next_transform.translation, interp_arg);
    interpolated.orientation = glm::slerp(prev_transform.orientation, next_transform.orientation, interp_arg);
    ret[bone] = interpolated.ToMatrix();
  }
  return ret;
}

AnimationTemplate::AnimationTemplate(const std::string& animation_path) {
  std::string full_path = std::string(kAnimationPathPrefix) + animation_path + ".fb";
  animation_raw_buffer_ = ReadWholeFile(full_path);
  animation_data_ = data::GetAnimation(animation_raw_buffer_.data());
  LOG_INFO("Animation loaded: %", animation_data_->path()->str());
}
