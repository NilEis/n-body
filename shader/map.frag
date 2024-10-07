#version 450

#extension GL_GOOGLE_cpp_style_line_directive : enable
#ifdef GL_GOOGLE_cpp_style_line_directive
#line 2 "map.frag"
#endif
layout(location = 0) out vec4 FragColor;

#ifndef DEFINES_H
#include "../include/defines.h"
#endif//DEFINES_H

#ifndef SHADER_INCLUDES_GLSL
#include "shader_includes.glsl"
#endif//SHADER_INCLUDES_GLSL

void main() {
    FragColor = vec4(1.0);
}