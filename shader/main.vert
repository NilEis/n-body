#version 460

#extension GL_GOOGLE_cpp_style_line_directive : enable
#ifdef GL_GOOGLE_cpp_style_line_directive
#line 2 "main.vert"
#endif

layout(location = 0) in vec3 in_pos;

void main() {
  gl_Position = vec4(in_pos, 1.0);
}