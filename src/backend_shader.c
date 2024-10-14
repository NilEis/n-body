#include "backend_shader.h"
#include "log.h"

#include "shader.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

GLuint compile_shader (const char *src, const char *name, const GLenum type)
{
    GLint success;
    char *c_src = calloc (strlen (src) + 1, sizeof (char));
    strcpy (c_src, src);
    char *version_str = c_src;
    char *rest_of_src = c_src;
    while (*rest_of_src != '\0')
    {
        if (*rest_of_src != '\n')
        {
            rest_of_src++;
        }
        else
        {
            *rest_of_src = '\0';
            rest_of_src++;
            break;
        }
    }
    const char *srcs[] = { version_str,
        "\n#define INCLUDED_FROM_GLSL_CODE 1\n",
        shader_defines_h,
        "\n",
        shader_shader_includes_glsl,
        "\n",
        rest_of_src };

    LOG (LOG_INFO, "Compiling ");
    switch (type)
    {
    case GL_COMPUTE_SHADER:
        LOG (LOG_CONTINUE, "compute");
        break;
    case GL_VERTEX_SHADER:
        LOG (LOG_CONTINUE, "vertex");
        break;
    case GL_TESS_CONTROL_SHADER:
        LOG (LOG_CONTINUE, "tessellation control");
        break;
    case GL_TESS_EVALUATION_SHADER:
        LOG (LOG_CONTINUE, "tessellation evaluation");
        break;
    case GL_GEOMETRY_SHADER:
        LOG (LOG_CONTINUE, "geometry");
        break;
    case GL_FRAGMENT_SHADER:
        LOG (LOG_CONTINUE, "fragment");
        break;
    default:
        LOG (LOG_CONTINUE, "Unknown");
        break;
    }
    LOG (LOG_CONTINUE, " shader(%s)", name);

    // create shader object
    const GLuint shader = glCreateShader (type);
    glShaderSource (shader, 7, srcs, NULL);
    glCompileShader (shader);

    // check for shader compile errors
    glGetShaderiv (shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog (shader, 512, NULL, infoLog);
        LOG (LOG_CONTINUE, " - error\n");
        LOG (LOG_ERROR, " - Shader compilation failed: %s\n", infoLog);
        free (c_src);
        return 0;
    }
    LOG (LOG_CONTINUE, " - success\n");
    free (c_src);
    return shader;
}

GLuint link_shader_program (int num_shaders, const GLuint *shaders)
{
    GLint success;
    GLchar infoLog[512];
    LOG (LOG_INFO, "Linking shader program");
    GLuint program = glCreateProgram ();
    for (int i = 0; i < num_shaders; i++)
    {
        glAttachShader (program, shaders[i]);
    }
    glLinkProgram (program);
    glGetProgramiv (program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog (program, 512, NULL, infoLog);
        LOG (LOG_CONTINUE, " - error\n");
        LOG (LOG_ERROR, " - Program linking failed: %s\n", infoLog);
        program = 0;
    }
    else
    {
        LOG (LOG_CONTINUE, " - success\n");
    }
    for (int i = 0; i < num_shaders; i++)
    {
        glDeleteShader (shaders[i]);
    }
    return program;
}

GLuint vlink_shader_program (const int num_shaders, ...)
{
    va_list ap;
    GLuint *shaders = calloc (num_shaders, sizeof (GLuint));
    va_start (ap, num_shaders);
    for (int i = 0; i < num_shaders; i++)
    {
        shaders[i] = va_arg (ap, GLuint);
    }
    va_end (ap);
    const GLuint program = link_shader_program (num_shaders, shaders);
    free (shaders);
    return program;
}

GLuint create_shader_program (
    const int num_shaders, shader_source shader_srcs[])
{
    GLuint *shaders = calloc (num_shaders, sizeof (GLuint));
    GLuint program = 0;
    assert (shaders != NULL);
    for (int i = 0; i < num_shaders; i++)
    {
        shaders[i] = compile_shader (
            shader_srcs[i].src, shader_srcs[i].name, shader_srcs[i].type);
        if (shaders[i] == 0)
        {
            for (int j = 0; j < i; j++)
            {
                glDeleteShader (shaders[j]);
            }
            goto end;
        }
    }
    program = link_shader_program (num_shaders, shaders);
end:
    free (shaders);
    return program;
}