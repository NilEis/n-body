#version 460

#extension GL_GOOGLE_cpp_style_line_directive : enable
#ifdef GL_GOOGLE_cpp_style_line_directive
#line 2 "main.frag"
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
    ivec2 uv = ivec2((gl_FragCoord.xy / size) * vec2(imageSize(map)));
    float v = imageLoad(map, uv).r;
    vec3 w = imageLoad(gravity_map, uv).rgb;
    //v = v / (v + 1.0);
    FragColor = vec4(vec3(v)+w, 1.0);
}