#version 300 es

// 256 = 4096 components. GL_MAX_VERTEX_UNIFORM_COMPONENTS = 4096 on a MacBook.
// Only 1024 is guaranteed, so we really should be using a uniform block instead.
// TODO: switch to UBO for bone transforms.
const int kMaxBones = 192;

const int kMaxBoneInfluences = 4;
const int kNoInfluenceBone = 255;

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec2 v_tex_coords;
layout(location = 4) in vec2 v_ao_tex_coords;
layout(location = 5) in ivec4 v_bone_ids;
layout(location = 6) in vec4 v_bone_weights;

uniform mat4 mvp;
uniform mat4 model;

uniform int skinning;
uniform mat4 bone_transforms[kMaxBones];

#include "light.vinc"

void main() {
  vec4 total_position = vec4(0.0f);
  vec3 total_normal = vec3(0.0f);
  vec3 total_tangent = vec3(0.0f);

  if (skinning != 0) {
    for (int influence = 0; influence < kMaxBoneInfluences; ++influence) {
      if (v_bone_ids[influence] == kNoInfluenceBone) {
        continue;
      }
      mat4 bone_transform = bone_transforms[v_bone_ids[influence]];
      float weight = v_bone_weights[influence];

      total_position += weight * (bone_transform * vec4(v_position, 1.0f));
      total_normal += weight * (mat3(bone_transform) * v_normal);
      total_tangent += weight * (mat3(bone_transform) * v_tangent);
    }
  } else {
    total_position = vec4(v_position, 1.0f);
    total_normal = v_normal;
    total_tangent = v_tangent;
  }

  gl_Position = mvp * total_position;

  vec3 tangent = normalize((model * vec4(total_tangent, 0.0)).xyz);
  vec3 bitangent = normalize((model * vec4(cross(total_normal, total_tangent), 0.0f)).xyz);
  vec3 normal = normalize(vec4(model * vec4(total_normal, 0.0)).xyz);

  set_tex_coords(v_tex_coords);

  set_ao_tex_coords(v_ao_tex_coords);

  set_normal(normal);

  set_tbn(mat3(tangent, bitangent, normal));

  compute_light_space((model * total_position).xyz);

  compute_light_outputs((model * total_position).xyz, vec3(0.0f, 0.0f, 1.0f));
}
