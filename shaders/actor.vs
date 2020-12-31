#version 300 es

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec2 v_tex_coords;
layout(location = 4) in vec2 v_ao_tex_coords;

uniform mat4 mvp;
uniform mat4 model;
uniform mediump mat3 normal_matrix;

uniform vec3 light_pos;
uniform vec3 eye_pos;

out vec2 tex_coords;
out vec2 ao_tex_coords;
out vec3 normal_interpo;
out mat3 tbn;

// Do all computations that need highp in vertex shader because
// some devices don't support highp in fragment shader.
// It's technically slightly wrong to normalize directions per-vertex
// instead of per-pixel, but it's close enough.
out vec3 norm_world_to_light;
out vec3 norm_world_to_eye;

void main() {
  gl_Position = mvp * vec4(v_position, 1.0);
  vec3 tangent = normalize(vec3(normal_matrix * v_tangent));
  vec3 bitangent = normalize(vec3(normal_matrix * cross(v_normal, v_tangent)));
  vec3 normal = normalize(vec3(normal_matrix * v_normal));
  tbn = mat3(tangent, bitangent, normal);

  tex_coords = v_tex_coords;
  ao_tex_coords = v_ao_tex_coords;
  normal_interpo = normal;

  vec3 world_pos = (model * vec4(v_position, 1.0)).xyz;
  norm_world_to_light = normalize(light_pos - world_pos);
  norm_world_to_eye = normalize(eye_pos - world_pos);
}
