#version 460

#extension GL_GOOGLE_cpp_style_line_directive : enable
#ifdef GL_GOOGLE_cpp_style_line_directive
#line 2 "main.frag"
#endif

layout(location = 0) out vec4 FragColor;


layout(binding = 0) uniform uniforms_buffer {
    vec2 size;
    int time;
};

layout(std430, binding = 1) buffer ant_buffer {
    vec3 ants[];
};

layout(binding = 2,R32F) uniform image2D map;

void main() {
    ivec2 uv = ivec2((gl_FragCoord.xy / size) * vec2(imageSize(map)));
    float v = imageLoad(map, uv).r;
    //v = v / (v + 1.0);
    FragColor = vec4(vec3(v), 1.0);
}