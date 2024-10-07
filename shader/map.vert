#version 460

#extension GL_GOOGLE_cpp_style_line_directive : enable
#ifdef GL_GOOGLE_cpp_style_line_directive
#line 2 "map.vert"
#endif

#ifndef MAP_WIDTH
#define MAP_WIDTH (1920/4)
#endif

#ifndef MAP_HEIGHT
#define MAP_HEIGHT (1080/4)
#endif

layout(location = 0) in vec3 in_pos;

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