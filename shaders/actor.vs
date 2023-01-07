#version 300 es

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec2 v_tex_coords;
layout(location = 4) in vec2 v_ao_tex_coords;
layout(location = 5) in ivec4 v_bone_ids;
layout(location = 6) in vec4 v_bone_weights;

uniform mat4 mvp;
uniform mat4 model;

#include "light.vinc"

#include "skinning.vinc"

void main() {
  SkinnedResult skinned =
      MaybeSkinPositionNormalTangent(v_position, v_normal, v_tangent, v_bone_ids, v_bone_weights);

  gl_Position = mvp * skinned.position;

  vec3 tangent = normalize((model * vec4(skinned.tangent, 0.0)).xyz);
  vec3 bitangent = normalize((model * vec4(cross(skinned.normal, skinned.tangent), 0.0f)).xyz);
  vec3 normal = normalize(vec4(model * vec4(skinned.normal, 0.0)).xyz);

  set_tex_coords(v_tex_coords);

  set_ao_tex_coords(v_ao_tex_coords);

  set_normal(normal);

  set_tbn(mat3(tangent, bitangent, normal));

  compute_light_space((model * skinned.position).xyz);

  compute_light_outputs((model * skinned.position).xyz, vec3(0.0f, 0.0f, 1.0f));
}
