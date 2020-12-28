#version 300 es

precision mediump float;

uniform bool is_edge;

out vec4 frag_colour;

void main() {
  if (is_edge) {
    frag_colour = vec4(0.0, 0.7, 0.0, 1.0);
  } else {
    frag_colour = vec4(0.0, 1.0, 0.0, 1.0);
  }
}
