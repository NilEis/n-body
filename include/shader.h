#ifndef SHADER_H
#define SHADER_H

#include <stddef.h>

#define SHADER_fs "#version 330 core\n"\
"\n"\
"out vec4 FragColor;\n"\
"\n"\
"void main() {\n"\
"    FragColor = vec4(gl_FragCoord.xyz,1.0);\n"\
"}"
const char const *basic_shader_fs = SHADER_fs;
const size_t basic_shader_fs_size = sizeof(SHADER_fs) / sizeof(SHADER_fs[0]);
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
const size_t basic_shader_vs_size = sizeof(SHADER_vs) / sizeof(SHADER_vs[0]);
#undef SHADER_vs

#define SHADER_SOLVE_comp ""
const char const *shader_solve_comp = SHADER_SOLVE_comp;
const size_t shader_solve_comp_size = sizeof(SHADER_SOLVE_comp) / sizeof(SHADER_SOLVE_comp[0]);
#undef SHADER_SOLVE_comp

#define SHADER_UPDATE_comp ""
const char const *shader_update_comp = SHADER_UPDATE_comp;
const size_t shader_update_comp_size = sizeof(SHADER_UPDATE_comp) / sizeof(SHADER_UPDATE_comp[0]);
#undef SHADER_UPDATE_comp

#endif // SHADER_H
