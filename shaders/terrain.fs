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

// Normal is always up for now because we have flat terrain.
const vec3 normal_interop = vec3(0.0f, 0.0f, 1.0f);

uniform bool is_edge;
uniform bool use_lighting;
uniform bool use_specular_highlight;
uniform bool use_normal_map;

uniform sampler2D base_texture;
uniform sampler2D spec_texture;
uniform sampler2D norm_texture;

out vec4 frag_colour;

void main() {
  vec3 colour = texture(base_texture, tex_coords).rgb;
  vec3 normal = normal_interop;

  if (use_normal_map) {
    normal = normalize(kTbn * (texture(norm_texture, tex_coords).xyz * 2.0f - 1.0f));
  }

  if (use_lighting) {
    vec3 norm = normalize(normal);
    float diffuse_factor = max(dot(norm, norm_world_to_light), 0.0f) * kDirectionalLightIntensity;
    vec3 diffuse = diffuse_factor * colour;

    vec3 spec = vec3(0.0, 0.0, 0.0);
    if (use_specular_highlight) {
      vec4 s = texture(spec_texture, tex_coords);
      vec3 spec_colour = s.rgb;
      vec3 reflect_dir = reflect(-norm_world_to_light, norm);
      float spec_power = pow(max(dot(norm_world_to_eye, reflect_dir), 0.0), kShininess);
      spec = spec_colour * spec_power;
    }

    vec3 ambient = kAmbientLight * colour;

    colour = clamp(diffuse + ambient + spec, 0.0, 1.0);
  }

  if (is_edge) {
  	colour = colour * 0.8f;
  }

  frag_colour = vec4(colour, 1.0f);
}
