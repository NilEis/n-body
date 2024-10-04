#include "backend.h"
#include "defines.h"
#include "glad/gl.h"
#include "shader.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_ERROR "ERROR: "
#define LOG_INFO "INFO:  "

#define LOG(a, str, ...) printf (a str, ##__VA_ARGS__)

#define SIZE_ELEM 2

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
    GLuint comp_shader_blur;
    GLuint shader_map;
    GLuint comp_shader_sub;
    struct
    {
        GLuint shader;
        char *name;
        GLuint x;
        GLuint y;
        GLuint z;
        bool swap_maps;
    } compute_shader_pipeline[NUM_COMP_SHADERS];
    GLuint map_a;
    GLuint map_a_framebuffer;
    GLuint map_b;
    GLuint map_b_framebuffer;
    GLuint active_framebuffer;
    bool current_map_is_a;
    GLuint ssbo;
    GLint time;
    GLint selected_ant;
    int render_passes_n;
    vertex_data_t vertex_data;
    vertex_data_t point_vx_data;
    GLfloat size[2];
    GLfloat map_size[4];
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
    GLfloat ants_pos[SIZE_ELEM * NUM_ANTS];
    struct
    {
        GLfloat *x;
        GLfloat *y;
        double vx;
        double vy;
        double fx;
        double fy;
        double w;
        double kin;
        double pot;
    } ants[NUM_ANTS];
} state_t;

static void error_callback (int error, const char *description);

GLuint create_shader_prog_string (
    const char *basic_shader_fs_string, const char *basic_shader_vs_string);
GLuint create_compute_shader_prog_spirv (
    const uint32_t *compute_bin, GLsizei compute_bin_len);
GLuint create_compute_shader_prog_string (const char *src);
GLuint create_uniform_buffer (void);

vertex_data_t init_vertex_data (
    const float (*vertices)[], GLsizeiptr size, int num);

static GLFWwindow *init_glfw (void);

bool swap_textures (bool current_map_is_a);

static void cleanup_glfw (state_t *s);

void print_uniforms (GLuint program);

void framebuffer_size_callback (GLFWwindow *window, int width, int height);

static state_t state = { 0 };

GLuint create_texture_2d ();
GLuint create_framebuffer (GLuint texture);
GLuint create_ssbo (void *p, size_t size, int flags);

int backend_init (void)
{
    state.window = init_glfw ();
    state.window_name[0] = 'A';
    state.window_name[1] = 'n';
    state.window_name[2] = 't';
    state.window_name[3] = 's';
    state.window_name[4] = '\0';
    state.selected_ant = 0;
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
    state.comp_shader_blur
        = create_compute_shader_prog_string (shader_blur_comp);
    state.shader_map
        = create_shader_prog_string (shader_map_frag, shader_map_vert);
    {
        static constexpr float vertices[] = {
            -1.0f,
            -1.0f,
            0.0f, // v1
            4.0f,
            -1.0f,
            0.0f, // v2
            -1.0f,
            4.0f,
            0.0f, // v3
        };
        state.vertex_data = init_vertex_data (&vertices, sizeof (vertices), 3);
    }
    {
        static constexpr float vertices[] = { 0.0f, 0.0f, 1.0f };
        state.point_vx_data
            = init_vertex_data (&vertices, sizeof (vertices), 1);
    }
    state.size[0] = (GLfloat)WINDOW_WIDTH;
    state.size[1] = (GLfloat)WINDOW_HEIGHT;
    state.map_size[0] = (GLfloat)0;
    state.map_size[1] = (GLfloat)0;
    state.map_size[2] = (GLfloat)MAP_WIDTH;
    state.map_size[3] = (GLfloat)MAP_HEIGHT;
    print_uniforms (state.shader);
    state.uniforms_buffer_object.ubo = create_uniform_buffer ();
    state.map_a = create_texture_2d ();
    state.map_b = create_texture_2d ();
    {
        LOG (LOG_INFO, "Generating particles\n");
        memset (state.ants_pos, 0, sizeof (state.ants_pos));
        for (int i = 0; i < SIZE_ELEM * NUM_ANTS; i += SIZE_ELEM)
        {
            const GLfloat r = rand ();
            GLfloat d = rand () % (int)(MAP_HEIGHT / 4);
            d = d == 0 ? 1 : d;
            const GLfloat x = cos (r);
            const GLfloat y = sin (r);
            state.ants_pos[i + 0] = x * d;
            state.ants_pos[i + 1] = y * d;
        }
    }
    state.ssbo = create_ssbo (
        state.ants_pos, sizeof (state.ants_pos), GL_DYNAMIC_STORAGE_BIT);
    {
        int i = 0;
        {
            state.compute_shader_pipeline[i].shader = state.comp_shader_sub;
            state.compute_shader_pipeline[i].name = "sub";
            state.compute_shader_pipeline[i].x = (GLuint)MAP_WIDTH;
            state.compute_shader_pipeline[i].y = (GLuint)MAP_HEIGHT;
            state.compute_shader_pipeline[i].z = 1;
            state.compute_shader_pipeline[i].swap_maps = true;
        }
        i++;
        {
            state.compute_shader_pipeline[i].shader = state.comp_shader_blur;
            state.compute_shader_pipeline[i].name = "blur";
            state.compute_shader_pipeline[i].x = (GLuint)MAP_WIDTH;
            state.compute_shader_pipeline[i].y = (GLuint)MAP_HEIGHT;
            state.compute_shader_pipeline[i].z = 1;
            state.compute_shader_pipeline[i].swap_maps = true;
        }
    }
    LOG (LOG_INFO, "Compute shader pipeline:\n");
    for (int i = 0; i < NUM_COMP_SHADERS; i++)
    {
        LOG (LOG_INFO, " - %s\n", state.compute_shader_pipeline[i].name);
    }
    glBindImageTexture (
        2, state.map_a, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
    glBindImageTexture (
        3, state.map_b, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
    state.active_framebuffer = state.map_b_framebuffer;
    state.current_map_is_a = true;
    for (int i = 0; i < NUM_ANTS; i++)
    {
        state.ants[i].x = &state.ants_pos[i * SIZE_ELEM];
        state.ants[i].y = &state.ants_pos[i * SIZE_ELEM + 1];
        state.ants[i].vx = 0.0;
        state.ants[i].vy = 0.0;
        state.ants[i].fx = 0;
        state.ants[i].fy = 0;
        state.ants[i].w = 2.0E14;
        state.ants[i].kin = 0;
        state.ants[i].pot = 0;
    }
    state.ants[0].w = 2.0E12;
    state.ants[0].vx = 0;
    state.ants[0].vy = 0;
    {
        GLfloat max_wheight = 0.0;
        for (int i = 0; i < NUM_ANTS; i++)
        {
            if (max_wheight < state.ants[i].w)
            {
                max_wheight = state.ants[i].w;
            }
        }
        memcpy (state.uniforms_buffer_object.mem
                    + state.uniforms_buffer_object.offset[2],
            &(max_wheight),
            sizeof (GLfloat));
        glBufferSubData (GL_UNIFORM_BUFFER,
            0,
            state.uniforms_buffer_object.block_size,
            state.uniforms_buffer_object.mem);
    }
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
    state.time++;
    memcpy (state.uniforms_buffer_object.mem
                + state.uniforms_buffer_object.offset[3],
        &(state.time),
        sizeof (GLint));
    memcpy (state.uniforms_buffer_object.mem
                + state.uniforms_buffer_object.offset[4],
        &(state.selected_ant),
        sizeof (GLint));
    glBufferSubData (GL_UNIFORM_BUFFER,
        0,
        state.uniforms_buffer_object.block_size,
        state.uniforms_buffer_object.mem);

    glBufferSubData (GL_SHADER_STORAGE_BUFFER,
        0,
        SIZE_ELEM * NUM_ANTS * sizeof (GLfloat),
        state.ants_pos);
    glBindVertexArray (state.point_vx_data.VAO);
    glBindFramebuffer (GL_FRAMEBUFFER, state.active_framebuffer);
    glViewport (-MAP_WIDTH / 2, -MAP_HEIGHT / 2, MAP_WIDTH, MAP_HEIGHT);
    glUseProgram (state.shader_map);
    glDrawArraysInstanced (GL_POINTS, 0, 1, NUM_ANTS);
    state.current_map_is_a = swap_textures (state.current_map_is_a);

    for (int i = 0; i < NUM_COMP_SHADERS; i++)
    {
        glBindFramebuffer (GL_FRAMEBUFFER, state.active_framebuffer);
        glUseProgram (state.compute_shader_pipeline[i].shader);
        glDispatchCompute (state.compute_shader_pipeline[i].x,
            state.compute_shader_pipeline[i].y,
            state.compute_shader_pipeline[i].z);
        glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        if (state.compute_shader_pipeline[i].swap_maps)
        {
            state.current_map_is_a = swap_textures (state.current_map_is_a);
        }
    }
    glBindFramebuffer (GL_FRAMEBUFFER, 0);
    glBindVertexArray (state.vertex_data.VAO);
    glUseProgram (state.shader);
    glDrawArrays (GL_TRIANGLES, 0, 3);
    state.current_map_is_a = swap_textures (state.current_map_is_a);
    // Swap buffers and poll for events
    glfwSwapBuffers (state.window);
    glfwSetWindowTitle (state.window, state.window_name);
    // sleep(1);
}

void update (void)
{
    for (int i = 0; i < NUM_ANTS; i++)
    {
        state.ants[i].fx = 0;
        state.ants[i].fy = 0;
    }
    for (int i = 0; i < NUM_ANTS; i++)
    {
        const double i_w = state.ants[i].w;
        for (int j = i + 1; j < NUM_ANTS; j++)
        {
            const double dx = *state.ants[j].x - *state.ants[i].x;
            const double dy = *state.ants[j].y - *state.ants[i].y;
            const double j_w = state.ants[j].w;
            double dist = sqrt (dx * dx + dy * dy);
            if (dist < EPSILON)
            {
                dist = EPSILON;
            }
            const double F
                = (GRAVITATIONAL_CONSTANT * i_w * j_w) / (dist * dist);
            const double force_x = F * dx / dist;
            state.ants[i].fx += force_x;
            const double force_y = F * dy / dist;
            state.ants[i].fy += force_y;
            state.ants[j].fx -= force_x;
            state.ants[j].fy -= force_y;
        }
    }
    for (auto i = 0; i < NUM_ANTS; i++)
    {
        const double i_w = state.ants[i].w;
        state.ants[i].vx += state.ants[i].fx / i_w;
        state.ants[i].vy += state.ants[i].fy / i_w;
        *state.ants[i].x += state.ants[i].vx;
        *state.ants[i].y += state.ants[i].vy;
    }
}

GLFWwindow *backend_get_window (void) { return state.window; }

void backend_deinit (void) { cleanup_glfw (&state); }

bool swap_textures (bool current_map_is_a)
{
    if (current_map_is_a)
    {
        glBindImageTexture (
            2, state.map_b, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
        glBindImageTexture (
            3, state.map_a, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
        state.active_framebuffer = state.map_a_framebuffer;
    }
    else
    {
        glBindImageTexture (
            2, state.map_a, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
        glBindImageTexture (
            3, state.map_b, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
        state.active_framebuffer = state.map_b_framebuffer;
    }
    return !current_map_is_a;
}

GLuint create_ssbo (void *p, size_t size, int flags)
{
    GLuint SSBO;
    glCreateBuffers (1, &SSBO);
    glNamedBufferStorage (SSBO, size, p, flags);
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, 0);
    return SSBO;
}

GLuint create_texture_2d ()
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

GLuint create_framebuffer (GLuint texture)
{
    GLuint FramebufferName = 0;
    glGenFramebuffers (1, &FramebufferName);
    glBindFramebuffer (GL_FRAMEBUFFER, FramebufferName);
    glBindTexture (GL_TEXTURE_2D, texture);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glFramebufferTexture (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers (1, DrawBuffers);
    return FramebufferName;
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

/**
 * compiles a compute shader
 * @param src the shader source
 * @return the handle for the linked program, 0 on error
 * @remark "#version" has to be in the first line
 */
GLuint create_compute_shader_prog_string (const char *src)
{
    GLuint computeShader, program;
    GLint success;
    GLchar infoLog[512];
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
    const char *srcs[]
        = { version_str, "\n", shader_defines_h, "\n", rest_of_src };

    // create shader object
    computeShader = glCreateShader (GL_COMPUTE_SHADER);
    glShaderSource (computeShader, 5, srcs, nullptr);
    glCompileShader (computeShader);

    // check for shader compile errors
    glGetShaderiv (computeShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog (computeShader, 512, nullptr, infoLog);
        LOG (LOG_ERROR, "Shader compilation failed: %s\n", infoLog);
        free (c_src);
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
        glGetProgramInfoLog (program, 512, nullptr, infoLog);
        LOG (LOG_ERROR, "Program linking failed: %s\n", infoLog);
        free (c_src);
        return 0;
    }

    // clean up
    glDeleteShader (computeShader);

    free (c_src);
    return program;
}

vertex_data_t init_vertex_data (
    const float (*vertices)[], GLsizeiptr size, int num)
{
    // Generate the VBO
    GLuint VBO;
    GLuint VAO;
    glGenBuffers (1, &VBO);
    glGenVertexArrays (1, &VAO);
    glBindVertexArray (VAO);
    glBindBuffer (GL_ARRAY_BUFFER, VBO);
    glBufferData (GL_ARRAY_BUFFER, size, *vertices, GL_STATIC_DRAW);
    glVertexAttribPointer (
        0, num, GL_FLOAT, GL_FALSE, 3 * sizeof (float), nullptr);
    glEnableVertexAttribArray (0);
    return (vertex_data_t){ .VBO = VBO, .VAO = VAO };
}

void cleanup_glfw (state_t *s)
{
    for (int i = 0; i < NUM_UNIFORMS; i++)
    {
        free (s->uniforms_buffer_object.names[i]);
    }
    LOG (LOG_INFO, "Destroying buffers\n");
    glDeleteVertexArrays (1, &(s->vertex_data.VAO));
    glDeleteBuffers (1, &(s->vertex_data.VBO));
    glDeleteBuffers (1, &(s->uniforms_buffer_object.ubo));
    glDeleteBuffers (1, &(s->ssbo));
    LOG (LOG_INFO, "Destroying framebuffers\n");
    glDeleteFramebuffers (1, &s->map_a_framebuffer);
    glDeleteFramebuffers (1, &s->map_b_framebuffer);
    LOG (LOG_INFO, "Destroying textures\n");
    glDeleteTextures (1, &(s->map_a));
    glDeleteTextures (1, &(s->map_b));
    free (s->uniforms_buffer_object.mem);
    LOG (LOG_INFO, "Destroying shader\n");
    glDeleteProgram (s->shader);
    LOG (LOG_INFO, "Destroying window\n");
    glfwDestroyWindow (s->window);
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
                + state.uniforms_buffer_object.offset[3],
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