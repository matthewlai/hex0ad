#version 300 es

precision mediump float;

in vec3 world_pos;

uniform bool is_edge;

// How much texture coordinates should be scaled by. 1.0 means 1m = 1 repeat of the texture.
uniform float texture_scale;

uniform sampler2D base_texture;

out vec4 frag_colour;

void main() {
  vec4 colour = texture(base_texture, texture_scale * vec2(world_pos.x, world_pos.y));

  if (is_edge) {
    frag_colour = colour * 0.8;
  } else {
    frag_colour = colour;
  }
}
