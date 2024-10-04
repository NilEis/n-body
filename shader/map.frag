#version 460

#extension GL_GOOGLE_cpp_style_line_directive : enable
#ifdef GL_GOOGLE_cpp_style_line_directive
#line 2 "map.frag"
#endif

#ifndef MAP_WIDTH
#define MAP_WIDTH (1920/4)
#endif

#ifndef MAP_HEIGHT
#define MAP_HEIGHT (1080/4)
#endif

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform uniforms_buffer {
    vec2 size;
    vec4 map_size;
    float max_weight;
    int time;
    int field;
};

layout(std430, binding = 1) buffer ant_buffer {
    vec2 ants[];
};

layout(binding = 2, R32F) uniform image2D map;

layout(binding = 4, R32F) uniform image2D gravity_map;

void main() {
    FragColor = vec4(1.0);
}