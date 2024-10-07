#ifndef BACKEND_SHADER_H
#define BACKEND_SHADER_H
#include "glad/gl.h"
typedef struct
{
    GLenum type;
    const char *src;
} shader_source;

GLuint compile_shader (const char *src, const GLenum type);

GLuint link_shader_program (int num_shaders, const GLuint *shaders);

GLuint vlink_shader_program (const int num_shaders, ...);

GLuint create_compute_shader_prog_string (const char *src);

GLuint create_shader_prog_string (
    const char *basic_shader_fs_string, const char *basic_shader_vs_string);

GLuint create_shader_program (int num_shaders, shader_source shader_srcs[]);

#endif // BACKEND_SHADER_H
