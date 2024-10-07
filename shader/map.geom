#version 460

#extension GL_GOOGLE_cpp_style_line_directive : enable
#ifdef GL_GOOGLE_cpp_style_line_directive
#line 2 "main.geom"
#endif

#ifndef MAP_WIDTH
#define MAP_WIDTH (1920/4)
#endif

#ifndef MAP_HEIGHT
#define MAP_HEIGHT (1080/4)
#endif

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