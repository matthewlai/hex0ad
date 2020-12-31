#version 300 es

layout(location = 0) in vec3 v_position;

uniform mat4 mvp;

// How much texture coordinates should be scaled by. 1.0 means 1m = 1 repeat of the texture.
uniform float texture_scale;

#include "light.vinc"

void main() {
  gl_Position = mvp * vec4(v_position, 1.0);

  set_tex_coords(texture_scale * vec2(v_position.x, v_position.y));

  // Normal is always up for now because we have flat terrain.
  set_normal(vec3(0.0f, 0.0f, 1.0f));

  // TBN is always identity for now because we have flat terrain.
  set_tbn(mat3(1.0f));

  compute_light_space(v_position);

  compute_light_outputs(v_position, vec3(0.0f, 0.0f, 1.0f));
}
