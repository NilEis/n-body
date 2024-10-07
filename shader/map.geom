#version 450

#extension GL_GOOGLE_cpp_style_line_directive : enable
#ifdef GL_GOOGLE_cpp_style_line_directive
#line 2 "main.geom"
#endif

#ifndef DEFINES_H
#include "../include/defines.h"
#endif//DEFINES_H

#ifndef SHADER_INCLUDES_GLSL
#include "shader_includes.glsl"
#endif//SHADER_INCLUDES_GLSL

layout (points) in;
layout (triangle_strip, max_vertices = 3) out;

void main() {
    gl_Position = gl_in[0].gl_Position + vec4(-0.01, 0.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(0.01, 0.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(0, 0.01, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}