#version 450 core

layout(location = 0) out vec4 FragColor;

#ifndef DEFINES_H
#include "../include/defines.h"
#endif//DEFINES_H

#ifndef SHADER_INCLUDES_GLSL
#include "shader_includes.glsl"
#endif//SHADER_INCLUDES_GLSL

void main() {
    ivec2 uv = ivec2((gl_FragCoord.xy / size) * vec2(imageSize(map_in)));
    float v = imageLoad(map_in, uv).r;
    //v = v / (v + 1.0);
    FragColor = vec4(vec3(v), 1.0);
}