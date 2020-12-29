#version 300 es

layout(location = 0) in vec3 v_position;

uniform mat4 mvp;

// Pass the world pos to fragment shader for texture lookup.
out vec3 world_pos;

void main() {
  gl_Position = mvp * vec4(v_position, 1.0);
  world_pos = v_position;
}
