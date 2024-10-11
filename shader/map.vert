#version 450 core

#line 2 "map.vert"

layout(location = 0) in vec3 in_pos;

#ifndef DEFINES_H
#include "../include/defines.h"
#endif//DEFINES_H

#ifndef SHADER_INCLUDES_GLSL
#include "./shader_includes.glsl"
#endif//SHADER_INCLUDES_GLSL

void main() {
    vec3 p = in_pos + vec3(ants[gl_InstanceID].xy, 0.0);
    p.xy /= size;
    gl_Position = vec4(p, 1.0);
}