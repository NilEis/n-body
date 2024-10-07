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

float map_value(float v, vec2 in_range, vec2 out_range)
{
    float slope = 1.0 * (out_range.y - out_range.x) / (in_range.y - in_range.x);
    return out_range.x + slope * (v - in_range.x);
}

void main() {
    vec3 p = in_pos+vec3(ants[gl_InstanceID].xy, 0.0);
    p.xy /= vec2(MAP_WIDTH, MAP_HEIGHT);
    p.xy -= 0.5;
    gl_Position = vec4(p, 1.0);
}