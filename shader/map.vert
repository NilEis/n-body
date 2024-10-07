#version 450

#extension GL_GOOGLE_cpp_style_line_directive : enable
#ifdef GL_GOOGLE_cpp_style_line_directive
#line 2 "map.vert"
#endif

layout(location = 0) in vec3 in_pos;

#ifndef DEFINES_H
#include "../include/defines.h"
#endif//DEFINES_H

#ifndef SHADER_INCLUDES_GLSL
#include "shader_includes.glsl"
#endif//SHADER_INCLUDES_GLSL

void main() {
    vec3 p = in_pos+vec3(ants[gl_InstanceID].xy, 0.0);
    p.xy /= map_size.zw;
    p.xy -= 0.5;
    gl_Position = vec4(p, 1.0);
}