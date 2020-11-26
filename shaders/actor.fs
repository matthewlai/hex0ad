#version 300 es

precision mediump float;

// Light computation is all in world coordinates.
const vec3 kAmbientLight = vec3(0.3, 0.3, 0.3);

const float kDirectionalLightIntensity = 1.0;
const float kAoStrength = 1.0;

const float kShininess = 4.0;

in vec2 tex_coords;
in vec2 ao_tex_coords;
in vec3 interp_normal;
in mat3 tbn;
in vec3 world_pos;

uniform sampler2D base_texture;
uniform sampler2D norm_texture;
uniform sampler2D spec_texture;
uniform sampler2D ao_texture;

uniform vec3 player_colour;
uniform vec3 light_pos;
uniform vec3 eye_pos;

// Effects settings.
uniform bool use_lighting;
uniform bool use_normal_map;
uniform bool use_player_colour;
uniform bool use_specular_highlight;
uniform bool use_ao_map;

out vec4 frag_colour;

void main() {
  vec4 base_colour = texture(base_texture, tex_coords);

  vec3 normal = interp_normal;
  if (use_normal_map) {
    normal = normalize(tbn * (texture(norm_texture, tex_coords).xyz * 2.0f - 1.0f));
  }

  // Player colour computation:
  // https://wildfiregames.com/forum/topic/18340-lowpoly-tips/
  vec3 player_colour_mixed = base_colour.rgb * player_colour;

  vec3 mixed_colour = base_colour.rgb;

  if (use_player_colour) {
    mixed_colour = base_colour.rgb * base_colour.a + player_colour_mixed * (1.0 - base_colour.a);
  }

  if (use_lighting) {
    vec3 light_dir = normalize(light_pos - world_pos);
    vec3 norm = normalize(normal);
    float diffuse_factor = max(dot(norm, light_dir), 0.0f) * kDirectionalLightIntensity;
    vec3 diffuse = diffuse_factor * mixed_colour;

    vec3 ambient = kAmbientLight * mixed_colour;

    if (use_ao_map) {
      vec3 ao = texture2D(ao_texture, ao_tex_coords).rrr;
      ao = mix(vec3(1.0), ao * 2.0, kAoStrength);
      ambient *= ao;
    }

    vec3 spec = vec3(0.0, 0.0, 0.0);
    if (use_specular_highlight) {
      vec4 s = texture(spec_texture, tex_coords);
      vec3 spec_colour = s.rgb;
      vec3 view_dir = normalize(eye_pos - world_pos);
      vec3 reflect_dir = reflect(-light_dir, norm);
      float spec_power = pow(max(dot(view_dir, reflect_dir), 0.0), kShininess);
      spec = spec_colour * spec_power;
    }

    frag_colour = vec4(clamp(diffuse + ambient + spec, 0.0, 1.0), 1.0);
  } else {
    frag_colour = vec4(mixed_colour, 1.0);
  }
}
