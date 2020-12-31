#version 300 es

// Pass-through shader for screen space passes.

layout(location = 0) in vec2 v_position;

out vec2 tex_coords;

void main() {
  tex_coords = v_position.xy;
  gl_Position = vec4(v_position * 2.0f - vec2(1.0f, 1.0f), 0.0f, 1.0f);
}
