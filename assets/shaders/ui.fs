#version 300 es

precision mediump float;

in vec2 tex_coords;

uniform sampler2D base_texture;

uniform int texture_byte_order;
// texture_byte_order:
// 0 = RGBA
// 1 = BGRA
// 2 = ARGB

out vec4 frag_colour;

void main() {
  vec4 tex = texture(base_texture, tex_coords);

  if (texture_byte_order == 0) {
    tex = tex.rgba;
  } else if (texture_byte_order == 1) {
    tex = tex.bgra;
  } else if (texture_byte_order == 2) {
    tex = tex.argb;
  }
  
  frag_colour = tex;
}
