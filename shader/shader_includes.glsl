#ifndef SHADER_INCLUDES_GLSL
#define SHADER_INCLUDES_GLSL

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

layout(binding = 2, R32F) uniform image2D map_in;
layout(binding = 3, R32F) uniform image2D map_out;

#endif //SHADER_INCLUDES_GLSL
