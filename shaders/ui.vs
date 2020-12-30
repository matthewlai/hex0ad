#version 300 es

// The UI shader just draws a textured rectangle.

layout(location = 0) in vec2 v_position;

// Region of the screen to render.
uniform vec4 xywh;

// Region of the texture to read.
uniform vec4 tex_xywh;

out vec2 tex_coords;

void main() {
  float screen_x = (v_position.x * xywh[2] + xywh[0]) * 2.0f - 1.0f;
  float screen_y = (v_position.y * xywh[3] + xywh[1]) * 2.0f - 1.0f;

  float tex_x = v_position.x * tex_xywh[2] + tex_xywh[0];
  float tex_y = v_position.y * tex_xywh[3] + tex_xywh[1];

  tex_coords = vec2(tex_x, tex_y);
  gl_Position = vec4(screen_x, -screen_y, 0.0f, 1.0f);
}
