#ifndef SHADER_H
#define SHADER_H

#define SHADER_fs "#version 330 core\n"\
"\n"\
"out vec4 FragColor;\n"\
"\n"\
"void main() {\n"\
"    FragColor = vec4(gl_FragCoord.xyz,1.0);\n"\
"}"
const char const *basic_shader_fs = SHADER_fs;
#undef SHADER_fs

#define SHADER_vs "#version 330 core\n"\
"\n"\
"in vec4 vertexPosition;\n"\
"\n"\
"uniform mat4 model;\n"\
"uniform mat4 view;\n"\
"uniform mat4 projection;\n"\
"\n"\
"void main()\n"\
"{\n"\
"    gl_Position = projection * view * model * vertexPosition;;\n"\
"}"
const char const *basic_shader_vs = SHADER_vs;
#undef SHADER_vs

#endif // SHADER_H
