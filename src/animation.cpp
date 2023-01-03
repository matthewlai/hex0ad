#include "animation.h"

#include <cmath>
#include <fstream>
#include <random>
#include <stdexcept>
#include <vector>

#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"

#include "logger.h"
#include "utils.h"

namespace {
static constexpr const char* kAnimationPathPrefix = "assets/art/animation/";
}

std::vector<glm::mat4> Animation::Update() {
  float normalised_time = static_cast<float>((GetTimeUs() - start_time_us_) / 1000000.0f) / template_->Duration();
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
  // TODO: Implement interpolation.
  auto num_frames = animation_data_->num_frames();
  int frame_number = std::clamp<int>(std::round(normalised_time * num_frames), 0, num_frames - 1);
  int stride = animation_data_->num_bones() * 7;
  const float* frame_offset = animation_data_->bone_states()->data() + stride * frame_number;
  std::vector<glm::mat4> ret(animation_data_->num_bones());
  for (std::size_t bone = 0; bone < animation_data_->num_bones(); ++bone) {
    ret[bone] = ReadBoneTransform(frame_offset + bone * 7).ToMatrix();
  }
  return ret;
}

AnimationTemplate::AnimationTemplate(const std::string& animation_path) {
  std::string full_path = std::string(kAnimationPathPrefix) + animation_path + ".fb";
  animation_raw_buffer_ = ReadWholeFile(full_path);
  animation_data_ = data::GetAnimation(animation_raw_buffer_.data());
  LOG_INFO("Animation loaded: %", animation_data_->path()->str());
}
