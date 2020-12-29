#version 300 es

layout(location = 0) in vec3 v_position;

uniform mat4 mvp;

uniform highp vec3 light_pos;
uniform highp vec3 eye_pos;

// How much texture coordinates should be scaled by. 1.0 means 1m = 1 repeat of the texture.
uniform float texture_scale;

// Do all computations that need highp in vertex shader because
// some devices don't support highp in fragment shader.
// It's technically slightly wrong to normalize directions per-vertex
// instead of per-pixel, but it's close enough.
out vec3 norm_world_to_light;
out vec3 norm_world_to_eye;
out vec2 tex_coords;

void main() {
  gl_Position = mvp * vec4(v_position, 1.0);

  // We don't have a model matrix for ground. We pre-transform all
  // vertices so we can render the entire ground in one draw call.
  vec3 world_pos = v_position;
  norm_world_to_light = normalize(light_pos - world_pos);
  norm_world_to_eye = normalize(eye_pos - world_pos);
  tex_coords = texture_scale * vec2(world_pos.x, world_pos.y);
}
