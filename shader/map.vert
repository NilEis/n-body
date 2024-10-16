#version 450 core

layout(location = 0) in vec3 in_pos;

#ifndef DEFINES_H
#include "../include/defines.h"
#endif//DEFINES_H

#ifndef SHADER_INCLUDES_GLSL
#include "./shader_includes.glsl"
#endif//SHADER_INCLUDES_GLSL

float normalize_value(float x, vec2 range)
{
    return 2.0*((x - range.x) / (range.y - range.x))-1.0;
}

vec2 normalize_value(vec2 x, vec4 range)
{
    return vec2(normalize_value(x.x, range.xz), normalize_value(x.y, range.yw));
}

void main() {
    vec3 p = in_pos + vec3(ants[gl_InstanceID].xy, 0.0);
    p.xy = normalize_value(p.xy, map_size);
    gl_Position = vec4(p, 1.0);
}