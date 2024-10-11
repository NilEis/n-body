#version 450 core

#ifndef DEFINES_H
#include "../include/defines.h"
#endif//DEFINES_H

#ifndef SHADER_INCLUDES_GLSL
#include "shader_includes.glsl"
#endif//SHADER_INCLUDES_GLSL

layout(location = 0) in vec3 in_pos;

void main() {
  gl_Position = vec4(in_pos, 1.0);
}