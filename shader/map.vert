#version 450 core

layout(location = 0) in vec3 in_pos;
out vec3 col;

#ifndef DEFINES_H
#include "../include/defines.h"
#endif//DEFINES_H

#ifndef SHADER_INCLUDES_GLSL
#include "./shader_includes.glsl"
#endif//SHADER_INCLUDES_GLSL

double normalize_value(double x, dvec2 range)
{
    return 2.0*((x - range.x) / (range.y - range.x))-1.0;
}

vec2 normalize_value(dvec2 x, dvec4 range)
{
    return vec2(normalize_value(x.x, range.xz), normalize_value(x.y, range.yw));
}

void main() {
    if (gl_InstanceID==0)
    {
        gl_PointSize = 3;
        col = vec3(1.0);
    }
    else
    {
        gl_PointSize = 1;
        col = vec3(1.0);
    }
    vec3 p = in_pos + vec3(ants[gl_InstanceID].xy, 0.0);
    p.xy = normalize_value(p.xy, map_size);
    gl_Position = vec4(p, 1.0);
}