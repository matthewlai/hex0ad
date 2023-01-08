#version 300 es

precision mediump float;

const vec3 kAmbientLight = vec3(0.3, 0.3, 0.3);
const float kDirectionalLightIntensity = 1.2;
const float kShininess = 4.0;

uniform bool is_edge;

out vec4 frag_colour;

#include "light.finc"

void main() {
  vec4 colour = compute_lighting(/*use_alpha_colour=*/ false, /*ao_strength=*/0.0f, kAmbientLight, kDirectionalLightIntensity, kShininess);

  if (is_edge) {
  	colour = colour * 0.8f;
  }

  frag_colour = colour;
}
