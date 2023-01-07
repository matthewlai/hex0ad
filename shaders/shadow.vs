#version 300 es

layout(location = 0) in vec3 v_position;
layout(location = 1) in ivec4 v_bone_ids;
layout(location = 2) in vec4 v_bone_weights;

// This is mvp from light space.
uniform mat4 mvp;

#include "skinning.vinc"

void main() {
  gl_Position = mvp * MaybeSkinPosition(v_position, v_bone_ids, v_bone_weights);
}
