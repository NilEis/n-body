#ifndef SHADER_INCLUDES_GLSL
#define SHADER_INCLUDES_GLSL

layout(binding = 0) uniform uniforms_buffer {
    dvec2 size;
    dvec4 map_size;
    int time;
};

layout(std430, binding = 1) buffer ant_buffer {
    dvec2 ants[];
};

layout(binding = 2, rgba32f) restrict readonly uniform image2D map_in;
layout(binding = 3, rgba32f) restrict writeonly uniform image2D map_out;

#endif//SHADER_INCLUDES_GLSL
