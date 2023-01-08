#version 300 es

layout(location = 0) in vec4 v_position;

uniform mat4 mvp;

void main() {
  gl_Position = mvp * v_position;
}

