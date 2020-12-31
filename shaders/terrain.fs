#version 300 es

precision mediump float;

const vec3 kAmbientLight = vec3(0.3, 0.3, 0.3);
const float kDirectionalLightIntensity = 1.7;
const float kShininess = 4.0;

// tbn is just identity matrix since we have the same texture
// orientation for all tiles, and normal is always up for now.
const mat3 kTbn = mat3(1.0f);

in vec3 norm_world_to_light;
in vec3 norm_world_to_eye;
in vec2 tex_coords;
in vec4 light_space_pos;

// Normal is always up for now because we have flat terrain.
const vec3 normal_interop = vec3(0.0f, 0.0f, 1.0f);

uniform bool is_edge;
uniform bool use_lighting;
uniform bool use_specular_highlight;
uniform bool use_normal_map;

uniform sampler2D base_texture;
uniform sampler2D spec_texture;
uniform sampler2D norm_texture;

uniform sampler2D shadow_texture;

out vec4 frag_colour;

float shadow(vec4 shadow_frag_pos) {
  vec3 shadow_tex_coords = (light_space_pos.xyz / light_space_pos.w) * 0.5f + 0.5f;

  // TODO: is there a more efficient way to do this?
  if (shadow_tex_coords.x < 0.0f || shadow_tex_coords.x > 1.0f ||
      shadow_tex_coords.y < 0.0f || shadow_tex_coords.y > 1.0f) {
    return 0.0f;
  }

  float depth_in_map = texture(shadow_texture, shadow_tex_coords.xy).r;
  float current_depth = shadow_tex_coords.z;
  return current_depth > depth_in_map ? 1.0f : 0.0f;
}

void main() {
  vec3 colour = texture(base_texture, tex_coords).rgb;
  vec3 normal = normal_interop;

  float in_shadow = shadow(light_space_pos);

  if (use_normal_map) {
    normal = normalize(kTbn * (texture(norm_texture, tex_coords).xyz * 2.0f - 1.0f));
  }

  if (use_lighting) {
    vec3 norm = normalize(normal);
    float diffuse_factor = max(dot(norm, norm_world_to_light), 0.0f) * kDirectionalLightIntensity;
    vec3 diffuse = diffuse_factor * colour * (1.0f - in_shadow);

    vec3 spec = vec3(0.0, 0.0, 0.0);
    if (use_specular_highlight) {
      vec4 s = texture(spec_texture, tex_coords);
      vec3 spec_colour = s.rgb;
      vec3 reflect_dir = reflect(-norm_world_to_light, norm);
      float spec_power = pow(max(dot(norm_world_to_eye, reflect_dir), 0.0), kShininess);
      spec = spec_colour * spec_power * (1.0f - in_shadow);
    }

    vec3 ambient = kAmbientLight * colour;

    colour = clamp(diffuse + ambient + spec, 0.0, 1.0);
  }

  if (is_edge) {
  	colour = colour * 0.8f;
  }

  frag_colour = vec4(colour, 1.0f);
}
