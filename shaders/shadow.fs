#version 300 es

precision mediump float;

// Depth is written automatically, so nothing to do here.

out vec4 frag_colour;

void main() {
  frag_colour = vec4(vec3(gl_FragCoord.z), 1.0);
}