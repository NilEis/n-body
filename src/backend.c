#include "backend.h"
#include "defines.h"
#include "glad/gl.h"
#include "shader.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_ERROR "ERROR: "
#define LOG_INFO "INFO:  "

#define LOG(a, str, ...) printf (a str, ##__VA_ARGS__)

static const int SIZE_ELEM = 6; // glsl vec3[3] = float1,float2,float3,float4,
// float5,float6,float7,float8

typedef struct
{
    GLuint VBO;
    GLuint VAO;
} vertex_data_t;

typedef struct
{
    GLFWwindow *window;
    char window_name[28];
    GLuint shader;
    GLuint comp_shader_map;
    GLuint comp_shader_sub;
    GLuint map_a;
    GLuint map_b;
    GLuint ssbo;
    GLint time;
    int render_passes_n;
    vertex_data_t vertex_data;
    GLfloat size[2];
    struct
    {
        GLubyte *uniforms;
        GLint block_size;
        GLuint ubo;
        GLuint indices[NUM_UNIFORMS];
        GLuint offset[NUM_UNIFORMS];
        GLubyte *mem;
        GLuint index;
        char *names[NUM_UNIFORMS];
    } uniforms_buffer_object;
    GLfloat *ants;
} state_t;

static void error_callback (int error, const char *description);

GLuint create_shader_prog_string (
    const char *basic_shader_fs_string, const char *basic_shader_vs_string);
GLuint create_compute_shader_prog_spirv (
    const uint32_t *compute_bin, GLsizei compute_bin_len);
GLuint create_compute_shader_prog_string (const char *src);
GLuint create_uniform_buffer (void);

vertex_data_t init_vertex_data (void);

static GLFWwindow *init_glfw (void);

static void cleanup_glfw (state_t s);

void print_uniforms (GLuint program);

void framebuffer_size_callback (GLFWwindow *window, int width, int height);

static state_t state = { 0 };

GLuint create_texture ();
GLuint create_ssbo ();

int backend_init (void)
{
    state.window = init_glfw ();
    state.window_name[0] = 'A';
    state.window_name[1] = 'n';
    state.window_name[2] = 't';
    state.window_name[3] = 's';
    state.window_name[4] = '\0';
    if (state.window == NULL)
    {
        return -1;
    }
    LOG (LOG_INFO, "Init GLAD\n");
    gladLoadGL (glfwGetProcAddress);
    LOG (LOG_INFO, "Load shaders\n");
    state.render_passes_n = 1;
    state.shader
        = create_shader_prog_string (shader_main_frag, shader_main_vert);
    state.comp_shader_sub
        = create_compute_shader_prog_string (shader_sub_comp);
    state.comp_shader_map
        = create_compute_shader_prog_string (shader_map_comp);
    state.vertex_data = init_vertex_data ();
    state.size[0] = (GLfloat)WINDOW_WIDTH;
    state.size[1] = (GLfloat)WINDOW_HEIGHT;
    print_uniforms (state.shader);
    state.uniforms_buffer_object.ubo = create_uniform_buffer ();
    state.map_a = create_texture ();
    state.map_b = create_texture ();
    state.ssbo = create_ssbo ();
    state.time = 0;
    LOG (LOG_INFO, "finished init\n");
}

GLFWwindow *init_glfw (void)
{
    LOG (LOG_INFO, "Initialize glfw\n");
    if (!glfwInit ())
    {
        LOG (LOG_ERROR, "Could not initialize glfw\n");
        return NULL;
    }
    glfwSetErrorCallback (error_callback);
    // Create window
    LOG (LOG_INFO, "Initialize window\n");
    // glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    GLFWwindow *window
        = glfwCreateWindow (WINDOW_WIDTH, WINDOW_HEIGHT, "N-Body", NULL, NULL);
    glfwMakeContextCurrent (window);
    glfwSetFramebufferSizeCallback (window, framebuffer_size_callback);
    if (!window)
    {
        LOG (LOG_ERROR, "Could not create window or context\n");
        return NULL;
    }
    return window;
}

void draw (void)
{
    glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT);

    glBindBuffer (GL_UNIFORM_BUFFER, state.uniforms_buffer_object.ubo);
    glBindBufferBase (GL_UNIFORM_BUFFER,
        state.uniforms_buffer_object.index,
        state.uniforms_buffer_object.ubo);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, state.ssbo);
    glBindImageTexture (
        2, state.map_a, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
    glBindImageTexture (
        3, state.map_b, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

    glBindVertexArray (state.vertex_data.VAO);
    state.time++;
    memcpy (state.uniforms_buffer_object.mem
                + state.uniforms_buffer_object.offset[1],
        &(state.time),
        sizeof (GLint));
    glBufferSubData (GL_UNIFORM_BUFFER,
        0,
        state.uniforms_buffer_object.block_size,
        state.uniforms_buffer_object.mem);

    glBufferSubData (GL_SHADER_STORAGE_BUFFER,
        0,
        SIZE_ELEM * NUM_ANTS * sizeof (GLfloat),
        state.ants);

    glUseProgram (state.comp_shader_map);
    glDispatchCompute ((GLuint)NUM_ANTS, 1, 1);
    glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glUseProgram (state.comp_shader_sub);
    glDispatchCompute ((GLuint)MAP_WIDTH, (GLuint)MAP_HEIGHT, 1);
    glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glUseProgram (state.shader);
    glDrawArrays (GL_TRIANGLES, 0, 3);
    // Swap buffers and poll for events
    glfwSwapBuffers (state.window);
    glfwSetWindowTitle (state.window, state.window_name);
    // sleep(1);
}

void update (void)
{
    for (int i = 0; i < NUM_ANTS; i++)
    {
        const int index = 6 * i;
        state.ants[index] = state.ants[index + 2];
        state.ants[index + 1] = state.ants[index + 3];
    }
}

GLFWwindow *backend_get_window (void) { return state.window; }

void backend_deinit (void)
{
    cleanup_glfw (state);
    free (state.ants);
}

GLuint create_ssbo ()
{
    GLuint SSBO;
    glCreateBuffers (1, &SSBO);
    LOG (LOG_INFO, "Generating ants\n");
    state.ants = calloc (SIZE_ELEM * NUM_ANTS, sizeof (GLfloat));
    for (int i = 0; i < SIZE_ELEM * NUM_ANTS; i += SIZE_ELEM)
    {
        const GLfloat r = rand ();
        GLfloat d = rand () % (int)(MAP_HEIGHT / 2.5);
        d = d == 0 ? 1 : d;
        const GLfloat x = cos (r);
        const GLfloat y = sin (r);
        state.ants[i + 0] = MAP_WIDTH / 2 + x * d;
        state.ants[i + 1] = MAP_HEIGHT / 2 + y * d;
        state.ants[i + 2] = 0.1f;
        state.ants[i + 3] = 0.1f;
        state.ants[i + 4] = 10;
    }
    glNamedBufferStorage (SSBO,
        SIZE_ELEM * sizeof (GLfloat) * NUM_ANTS,
        state.ants,
        GL_DYNAMIC_STORAGE_BIT);
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, 0);
    return SSBO;
}

GLuint create_texture ()
{
    GLuint texture;
    glGenTextures (1, &texture);
    glBindTexture (GL_TEXTURE_2D, texture);
    GLfloat *tmp_map = calloc (MAP_WIDTH * MAP_HEIGHT, sizeof (GLfloat));
    glTexImage2D (GL_TEXTURE_2D,
        0,
        GL_R32F,
        MAP_WIDTH,
        MAP_HEIGHT,
        0,
        GL_RED,
        GL_FLOAT,
        tmp_map);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    free (tmp_map);
    return texture;
}

GLuint create_shader_prog_spirv (const uint32_t *fragment_bin,
    GLsizei fragment_bin_len,
    const uint32_t *vertex_bin,
    GLsizei vertex_bin_len)
{
    GLuint vertexShader = glCreateShader (GL_VERTEX_SHADER);
    glShaderBinary (1,
        &vertexShader,
        GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
        vertex_bin,
        vertex_bin_len);
    glSpecializeShaderARB (vertexShader, "main", 0, NULL, NULL);

    // check for vertex shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv (vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog (vertexShader, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Vertex shader compilation failed: %s\n", infoLog);
    }

    // compile fragment shader
    GLuint fragmentShader = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderBinary (1,
        &fragmentShader,
        GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
        fragment_bin,
        fragment_bin_len);
    glSpecializeShaderARB (fragmentShader, "main", 0, NULL, NULL);

    // check for fragment shader compilation errors
    glGetShaderiv (fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog (fragmentShader, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Fragment shader compilation failed: %s\n", infoLog);
    }

    // link shaders
    GLuint shaderProgram = glCreateProgram ();
    glAttachShader (shaderProgram, vertexShader);
    glAttachShader (shaderProgram, fragmentShader);
    glLinkProgram (shaderProgram);

    // check for linking errors
    glGetProgramiv (shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog (shaderProgram, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Shader program linking failed: %s\n", infoLog);
    }

    // delete the shaders as they're linked into our program now and no longer
    // necessary
    glDeleteShader (vertexShader);
    glDeleteShader (fragmentShader);

    return shaderProgram;
}

GLuint create_compute_shader_prog_spirv (
    const uint32_t *compute_bin, GLsizei compute_bin_len)
{
    GLuint compute_shader = glCreateShader (GL_COMPUTE_SHADER);
    glShaderBinary (1,
        &compute_shader,
        GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
        compute_bin,
        compute_bin_len);
    glSpecializeShaderARB (compute_shader, "main", 0, NULL, NULL);

    // check for vertex shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv (compute_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog (compute_shader, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Vertex shader compilation failed: %s\n", infoLog);
    }

    GLuint shaderProgram = glCreateProgram ();
    glAttachShader (shaderProgram, compute_shader);
    glLinkProgram (shaderProgram);

    // check for linking errors
    glGetProgramiv (shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog (shaderProgram, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Shader program linking failed: %s\n", infoLog);
    }

    // delete the shaders as they're linked into our program now and no longer
    // necessary
    glDeleteShader (compute_shader);

    return shaderProgram;
}

GLuint create_compute_shader_prog_string (const char *src)
{
    GLuint computeShader, program;
    GLint success;
    GLchar infoLog[512];

    // create shader object
    computeShader = glCreateShader (GL_COMPUTE_SHADER);
    glShaderSource (computeShader, 1, &src, NULL);
    glCompileShader (computeShader);

    // check for shader compile errors
    glGetShaderiv (computeShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog (computeShader, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Shader compilation failed: %s\n", infoLog);
        return 0;
    }

    // create program object
    program = glCreateProgram ();
    glAttachShader (program, computeShader);
    glLinkProgram (program);

    // check for program linking errors
    glGetProgramiv (program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog (program, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Program linking failed: %s\n", infoLog);
        return 0;
    }

    // clean up
    glDeleteShader (computeShader);

    return program;
}

vertex_data_t init_vertex_data (void)
{
    const static float vertices[] = {
        -1.0,
        -1.0,
        0.0, // v1
        4.0,
        -1.0,
        0.0, // v2
        -1.0,
        4.0,
        0.0, // v3
    };
    // Generate the VBO
    GLuint VBO;
    GLuint VAO;
    glGenBuffers (1, &VBO);
    glGenVertexArrays (1, &VAO);
    glBindVertexArray (VAO);
    glBindBuffer (GL_ARRAY_BUFFER, VBO);
    glBufferData (
        GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer (
        0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float), (void *)0);
    glEnableVertexAttribArray (0);
    return (vertex_data_t){ .VBO = VBO, .VAO = VAO };
}

void cleanup_glfw (state_t s)
{
    for (int i = 0; i < NUM_UNIFORMS; i++)
    {
        free (state.uniforms_buffer_object.names[i]);
    }
    LOG (LOG_INFO, "Destroying buffers\n");
    glDeleteVertexArrays (1, &(state.vertex_data.VAO));
    glDeleteBuffers (1, &(state.vertex_data.VBO));
    glDeleteBuffers (1, &(state.uniforms_buffer_object.ubo));
    glDeleteBuffers (1, &(state.ssbo));
    LOG (LOG_INFO, "Destroying textures\n");
    glDeleteTextures (1, &(state.map_a));
    glDeleteTextures (1, &(state.map_b));
    free (state.uniforms_buffer_object.mem);
    LOG (LOG_INFO, "Destroying shader\n");
    glDeleteProgram (state.shader);
    LOG (LOG_INFO, "Destroying window\n");
    glfwDestroyWindow (state.window);
    LOG (LOG_INFO, "Terminating glfw\n");
    glfwTerminate ();
}

static void error_callback (int error, const char *description)
{
    LOG (LOG_ERROR, "Error: %s\n", description);
}

GLuint create_shader_prog_string (
    const char *basic_shader_fs_string, const char *basic_shader_vs_string)
{
    GLuint vertexShader = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource (vertexShader, 1, &basic_shader_vs_string, NULL);
    glCompileShader (vertexShader);

    // check for vertex shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv (vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog (vertexShader, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Vertex shader compilation failed: %s\n", infoLog);
    }

    // compile fragment shader
    GLuint fragmentShader = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (fragmentShader, 1, &basic_shader_fs_string, NULL);
    glCompileShader (fragmentShader);

    // check for fragment shader compilation errors
    glGetShaderiv (fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog (fragmentShader, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Fragment shader compilation failed: %s\n", infoLog);
    }

    // link shaders
    GLuint shaderProgram = glCreateProgram ();
    glAttachShader (shaderProgram, vertexShader);
    glAttachShader (shaderProgram, fragmentShader);
    glLinkProgram (shaderProgram);

    // check for linking errors
    glGetProgramiv (shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog (shaderProgram, 512, NULL, infoLog);
        LOG (LOG_ERROR, "Shader program linking failed: %s\n", infoLog);
    }

    // delete the shaders as they're linked into our program now and no longer
    // necessary
    glDeleteShader (vertexShader);
    glDeleteShader (fragmentShader);
    return shaderProgram;
}

void framebuffer_size_callback (GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that
    // width and height will be significantly larger than specified on retina
    // displays.
    glViewport (0, 0, width, height);
    state.size[0] = width;
    state.size[1] = height;
    memcpy (
        state.uniforms_buffer_object.mem, state.size, 2 * sizeof (GLfloat));
}

GLuint create_uniform_buffer (void)
{
    glUseProgram (state.shader);

    GLuint num_blocks;
    glGetProgramiv (state.shader, GL_ACTIVE_UNIFORM_BLOCKS, &num_blocks);
    LOG (LOG_INFO, "Num uniforms: %u\n", num_blocks);

    state.uniforms_buffer_object.index
        = glGetUniformBlockIndex (state.shader, "uniforms_buffer");
    LOG (LOG_INFO, "Uniform index: %d\n", state.uniforms_buffer_object.index);

    glGetActiveUniformBlockiv (state.shader,
        state.uniforms_buffer_object.index,
        GL_UNIFORM_BLOCK_DATA_SIZE,
        &(state.uniforms_buffer_object.block_size));
    LOG (LOG_INFO,
        "Uniform size: %d\n",
        state.uniforms_buffer_object.block_size);
    state.uniforms_buffer_object.mem
        = calloc (1, state.uniforms_buffer_object.block_size);

    glGetUniformIndices (state.shader,
        NUM_UNIFORMS,
        (const GLchar *const *)(state.uniforms_buffer_object.names),
        state.uniforms_buffer_object.indices);
    glGetActiveUniformsiv (state.shader,
        NUM_UNIFORMS,
        state.uniforms_buffer_object.indices,
        GL_UNIFORM_OFFSET,
        state.uniforms_buffer_object.offset);
    /* GLboolean sorted = GL_FALSE;
    int n = NUM_UNIFORMS;
    while (!sorted)
    {
        sorted = GL_TRUE;
        for (int i = 1; i < n; i++)
        {
            if (state.uniforms_buffer_object.offset[i] <
    state.uniforms_buffer_object.offset[i - 1])
            {
                GLuint tmp_index = state.uniforms_buffer_object.indices[i - 1];
                GLuint tmp_offset = state.uniforms_buffer_object.offset[i - 1];
                char *tmp_char = state.uniforms_buffer_object.names[i - 1];

                state.uniforms_buffer_object.indices[i - 1] =
    state.uniforms_buffer_object.indices[i];
                state.uniforms_buffer_object.offset[i - 1] =
    state.uniforms_buffer_object.offset[i];
                state.uniforms_buffer_object.names[i - 1] =
    state.uniforms_buffer_object.names[i];

                state.uniforms_buffer_object.indices[i] = tmp_index;
                state.uniforms_buffer_object.offset[i] = tmp_offset;
                state.uniforms_buffer_object.names[i] = tmp_char;
                sorted = GL_FALSE;
            }
        }
    } */
    for (int i = 0; i < NUM_UNIFORMS; i++)
    {
        LOG (LOG_INFO,
            " - index %s: %d - offset: %d\n",
            state.uniforms_buffer_object.names[i],
            state.uniforms_buffer_object.indices[i],
            state.uniforms_buffer_object.offset[i]);
    }
    memcpy (state.uniforms_buffer_object.mem
                + state.uniforms_buffer_object.offset[0],
        state.size,
        2 * sizeof (GLfloat));
    memcpy (state.uniforms_buffer_object.mem
                + state.uniforms_buffer_object.offset[1],
        &(state.time),
        sizeof (GLint));

    GLuint ubo_handle;
    glGenBuffers (1, &ubo_handle);
    glBindBuffer (GL_UNIFORM_BUFFER, ubo_handle);
    glBufferData (GL_UNIFORM_BUFFER,
        state.uniforms_buffer_object.block_size,
        state.uniforms_buffer_object.mem,
        GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_UNIFORM_BUFFER,
        state.uniforms_buffer_object.index,
        state.uniforms_buffer_object.ubo);
    return ubo_handle;
}

void print_uniforms (GLuint program)
{
    GLint num_blocks = 0;
    glGetProgramInterfaceiv (
        program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &num_blocks);
    const GLenum block_properties[1] = { GL_NUM_ACTIVE_VARIABLES };
    const GLenum active_unif_prop[1] = { GL_ACTIVE_VARIABLES };
    const GLenum unif_properties[3] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };

    for (int blockIx = 0; blockIx < num_blocks; ++blockIx)
    {
        GLint num_active_unifs = 0;
        glGetProgramResourceiv (program,
            GL_UNIFORM_BLOCK,
            blockIx,
            1,
            block_properties,
            1,
            NULL,
            &num_active_unifs);

        if (!num_active_unifs)
            continue;

        GLint *block_unifs = calloc (num_active_unifs, sizeof (GLint));
        glGetProgramResourceiv (program,
            GL_UNIFORM_BLOCK,
            blockIx,
            1,
            active_unif_prop,
            num_active_unifs,
            NULL,
            &block_unifs[0]);
        LOG (LOG_INFO, "Uniforms:\n");
        for (int unifIx = 0; unifIx < num_active_unifs; ++unifIx)
        {
            GLint values[3];
            glGetProgramResourceiv (program,
                GL_UNIFORM,
                block_unifs[unifIx],
                3,
                unif_properties,
                3,
                NULL,
                values);
            state.uniforms_buffer_object.names[unifIx]
                = calloc (values[0], sizeof (char));
            glGetProgramResourceName (program,
                GL_UNIFORM,
                block_unifs[unifIx],
                values[0],
                NULL,
                state.uniforms_buffer_object.names[unifIx]);
            LOG (LOG_INFO,
                " - Name: %s\n",
                state.uniforms_buffer_object.names[unifIx]);
        }
        free (block_unifs);
    }
}