#version 300 es

precision mediump float;

// Light computation is all in world coordinates.
const vec3 kAmbientLight = vec3(0.5, 0.5, 0.5);

const float kDirectionalLightIntensity = 1.3;
const float kAoStrength = 1.0;

const float kShininess = 4.0;

uniform bool use_alpha_colour;

out vec4 frag_colour;

#include "light.finc"

void main() {
  frag_colour = compute_lighting(use_alpha_colour, /*ao_strength=*/kAoStrength, kAmbientLight, kDirectionalLightIntensity, kShininess);
}

