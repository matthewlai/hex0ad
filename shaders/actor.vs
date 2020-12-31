#version 300 es

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec2 v_tex_coords;
layout(location = 4) in vec2 v_ao_tex_coords;

uniform mat4 mvp;
uniform mat4 model;
uniform mediump mat3 normal_matrix;

#include "light.vinc"

void main() {
  gl_Position = mvp * vec4(v_position, 1.0);

  vec3 tangent = normalize(vec3(normal_matrix * v_tangent));
  vec3 bitangent = normalize(vec3(normal_matrix * cross(v_normal, v_tangent)));
  vec3 normal = normalize(vec3(normal_matrix * v_normal));

  set_tex_coords(v_tex_coords);

  set_ao_tex_coords(v_ao_tex_coords);

  set_normal(normal);

  set_tbn(mat3(tangent, bitangent, normal));

  compute_light_space((model * vec4(v_position, 1.0f)).xyz);

  compute_light_outputs((model * vec4(v_position, 1.0f)).xyz, vec3(0.0f, 0.0f, 1.0f));
}
