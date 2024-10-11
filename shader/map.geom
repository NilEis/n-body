#version 450 core

#ifndef DEFINES_H
#include "../include/defines.h"
#endif//DEFINES_H

#ifndef SHADER_INCLUDES_GLSL
#include "shader_includes.glsl"
#endif//SHADER_INCLUDES_GLSL

#define RES 8
#define CIRCLE_SIZE 0.005

layout (points) in;
layout (triangle_strip, max_vertices = RES*2+2) out;

const float PI = 3.1415926;

void main() {
    for (int i = 0; i <= RES; i++) {
        // Angle between each side in radians
        float ang = PI * 2.0 / float(RES) * i;

        // Offset from center of point (0.3 to accomodate for aspect ratio)
        vec4 offset = vec4(cos(ang) * CIRCLE_SIZE, -sin(ang) * CIRCLE_SIZE, 0.0, 0.0);
        gl_Position = gl_in[0].gl_Position + offset;
        EmitVertex();
        gl_Position = gl_in[0].gl_Position;
        EmitVertex();
    }

    EndPrimitive();
}