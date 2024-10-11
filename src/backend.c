#include "backend.h"
#include "backend_shader.h"
#include "defines.h"
#include "glad/gl.h"
#include "log.h"
#include "shader.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE_ELEM 2

#define INTERNAL_TEXTURE_FORMAT GL_RGBA32F

typedef struct
{
    GLuint VBO;
    GLuint VAO;
} vertex_data_t;

typedef struct
{
    Arena arena;
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
    ant ants[NUM_ANTS];
} state_t;

static void error_callback (int error, const char *description);

GLuint create_uniform_buffer (GLuint program);

vertex_data_t init_vertex_data (
    const float (*vertices)[], GLsizeiptr size, int num);

static GLFWwindow *init_glfw (void);

bool swap_textures (bool current_map_is_a);

static void cleanup_glfw (const state_t *s);

void print_uniforms (GLuint program);

void framebuffer_size_callback (GLFWwindow *window, int width, int height);

static state_t state;

GLuint create_texture_2d ();
GLuint create_framebuffer (GLuint texture);
GLuint create_ssbo (const void *p, GLsizeiptr size, int flags);

APIENTRY void opengl_error_callback (GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *user_param);

int backend_init (void)
{
    state.window = init_glfw ();
    state.window_name[0] = 'A';
    state.window_name[1] = 'n';
    state.window_name[2] = 't';
    state.window_name[3] = 's';
    state.window_name[4] = '\0';
    state.selected_ant = 0;
    if (state.window == nullptr)
    {
        return -1;
    }
    LOG (LOG_INFO, "Init GLAD\n");
    gladLoadGL (glfwGetProcAddress);
    glDebugMessageCallback (opengl_error_callback, nullptr);
    LOG (LOG_INFO, "Load shaders\n");
    state.render_passes_n = 1;
    state.shader = create_shader_program (2,
        (shader_source[]){
            { .type = GL_FRAGMENT_SHADER, .src = shader_main_frag },
            { .type = GL_VERTEX_SHADER,   .src = shader_main_vert }
    });
    if (state.shader == 0)
    {
        return -1;
    }
    state.comp_shader_sub = create_shader_program (1,
        (shader_source[]){
            { .type = GL_COMPUTE_SHADER, .src = shader_sub_comp }
    });
    if (state.comp_shader_sub == 0)
    {
        return -1;
    }
    state.comp_shader_blur = create_shader_program (1,
        (shader_source[]){
            { .type = GL_COMPUTE_SHADER, .src = shader_blur_comp }
    });
    if (state.comp_shader_blur == 0)
    {
        return -1;
    }
    state.shader_map = create_shader_program (3,
        (shader_source[]){
            { .type = GL_VERTEX_SHADER,   .src = shader_map_vert },
            { .type = GL_GEOMETRY_SHADER, .src = shader_map_geom },
            { .type = GL_FRAGMENT_SHADER, .src = shader_map_frag }
    });
    if (state.shader_map == 0)
    {
        return -1;
    }
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
        state.vertex_data = init_vertex_data (
            (const float (*)[]) & vertices, sizeof (vertices), 3);
    }
    {
        static constexpr float vertices[] = { 0.5f, 0.5f, 1.0f };
        state.point_vx_data = init_vertex_data (
            (const float (*)[]) & vertices, sizeof (vertices), 1);
    }
    state.size[0] = (GLfloat)WINDOW_WIDTH;
    state.size[1] = (GLfloat)WINDOW_HEIGHT;
    glViewport (0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    state.map_size[0] = (GLfloat)0;
    state.map_size[1] = (GLfloat)0;
    state.map_size[2] = (GLfloat)MAP_WIDTH;
    state.map_size[3] = (GLfloat)MAP_HEIGHT;
    LOG (LOG_INFO, "Main uniforms:\n");
    print_uniforms (state.shader);
    state.uniforms_buffer_object.ubo
        = create_uniform_buffer (state.shader_map);
    state.map_a = create_texture_2d ();
    state.map_a_framebuffer = create_framebuffer (state.map_a);
    state.map_b = create_texture_2d ();
    state.map_b_framebuffer = create_framebuffer (state.map_b);
    {
        LOG (LOG_INFO, "Generating particles\n");
        srand (0);
        memset (state.ants_pos, 0, sizeof (state.ants_pos));
        for (int i = 0; i < SIZE_ELEM * NUM_ANTS; i += SIZE_ELEM)
        {
            const GLfloat r = (GLfloat)rand ();
            GLfloat d = (GLfloat)(rand () % (MAP_HEIGHT / 2));
            d = d == 0 ? 1 : d;
            const GLfloat x = cosf (r);
            const GLfloat y = sinf (r);
            state.ants_pos[i + 0] = MAP_WIDTH / 2 + x * d;
            state.ants_pos[i + 1] = MAP_HEIGHT / 2 + y * d;
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
    glBindImageTexture (2,
        state.map_a,
        0,
        GL_FALSE,
        0,
        GL_READ_WRITE,
        INTERNAL_TEXTURE_FORMAT);
    glBindImageTexture (3,
        state.map_b,
        0,
        GL_FALSE,
        0,
        GL_READ_WRITE,
        INTERNAL_TEXTURE_FORMAT);
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
    }
    state.ants[0].w = 2.0E18;
    state.ants[0].vx = 0;
    state.ants[0].vy = 0;
    glBufferSubData (GL_UNIFORM_BUFFER,
        0,
        state.uniforms_buffer_object.block_size,
        state.uniforms_buffer_object.mem);

    state.time = 0;
    LOG (LOG_INFO, "finished init\n");
    return 0;
}

GLFWwindow *init_glfw (void)
{
    LOG (LOG_INFO, "Initialize glfw\n");
    if (!glfwInit ())
    {
        LOG (LOG_ERROR, "Could not initialize glfw\n");
        return nullptr;
    }
    glfwSetErrorCallback (error_callback);
    // Create window
    LOG (LOG_INFO, "Initialize window\n");
    // glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 5);
#if !NDEBUG
    glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow (
        WINDOW_WIDTH, WINDOW_HEIGHT, "N-Body", nullptr, nullptr);
    glfwMakeContextCurrent (window);
    glfwSetFramebufferSizeCallback (window, framebuffer_size_callback);
    if (!window)
    {
        LOG (LOG_ERROR, "Could not create window or context\n");
        return nullptr;
    }
    return window;
}

void draw (void)
{
    glBindFramebuffer (GL_FRAMEBUFFER, 0);
    glClearColor (0.0f, 1.0f, 1.0f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT);

    glBindBuffer (GL_UNIFORM_BUFFER, state.uniforms_buffer_object.ubo);
    glBindBufferBase (GL_UNIFORM_BUFFER,
        state.uniforms_buffer_object.index,
        state.uniforms_buffer_object.ubo);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, state.ssbo);
    state.time++;
    memcpy (state.uniforms_buffer_object.mem
                + state.uniforms_buffer_object.offset[TIME_UNIFORM_INDEX],
        &state.time,
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
    glViewport (0, 0, MAP_WIDTH, MAP_HEIGHT);
    glUseProgram (state.shader_map);
    glBindFramebuffer (GL_FRAMEBUFFER, state.active_framebuffer);
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
    glViewport (0, 0, state.size[0], state.size[1]);
    glUseProgram (state.shader);
    glDrawArrays (GL_TRIANGLES, 0, 3);
    state.current_map_is_a = swap_textures (state.current_map_is_a);
    // Swap buffers and poll for events
    glfwSwapBuffers (state.window);
    // sleep(1);
}

void update (void)
{
#if 0
    for (int i = 0; i < NUM_ANTS; i++)
    {
        state.ants[i].fx = 0;
        state.ants[i].fy = 0;
        for (int j = 0; j < NUM_ANTS; j++)
        {
            if (i == j)
            {
                continue;
            }
            const ant *b = &state.ants[j];
            apply_force (&state.ants[i], *b->x, *b->y, b->w);
        }
    }
    for (auto i = 0; i < NUM_ANTS; i++)
    {
        ant *v = &state.ants[i];
        update_ant (v);
    }
#else
    arena_reset (&state.arena);
    bh_tree tree;
    auto q = (quad){ .x = 0.0f, .y = 0.0f, .length = { .full = 1000000 } };
    q.length.half = q.length.full / 2.0f;
    bh_tree_init (&tree, &q, &state.arena);
    for (int i = 0; i < NUM_ANTS; i++)
    {
        if (quad_contains (&tree.quad, *state.ants[i].x, *state.ants[i].y))
        {
            bh_tree_insert (&tree, &state.ants[i]);
        }
    }
    for (int i = 0; i < NUM_ANTS; i++)
    {
        state.ants[i].fx = 0;
        state.ants[i].fy = 0;
        if (quad_contains (&tree.quad, *state.ants[i].x, *state.ants[i].y))
        {
            bh_tree_apply_force (&tree, &state.ants[i]);
        }
        update_ant (&state.ants[i]);
    }
    arena_free (&state.arena);
#endif
}

GLFWwindow *backend_get_window (void) { return state.window; }

void backend_deinit (void) { cleanup_glfw (&state); }

bool swap_textures (const bool current_map_is_a)
{
    if (current_map_is_a)
    {
        glBindImageTexture (2,
            state.map_b,
            0,
            GL_FALSE,
            0,
            GL_READ_WRITE,
            INTERNAL_TEXTURE_FORMAT);
        glBindImageTexture (3,
            state.map_a,
            0,
            GL_FALSE,
            0,
            GL_READ_WRITE,
            INTERNAL_TEXTURE_FORMAT);
        state.active_framebuffer = state.map_a_framebuffer;
    }
    else
    {
        glBindImageTexture (2,
            state.map_a,
            0,
            GL_FALSE,
            0,
            GL_READ_WRITE,
            INTERNAL_TEXTURE_FORMAT);
        glBindImageTexture (3,
            state.map_b,
            0,
            GL_FALSE,
            0,
            GL_READ_WRITE,
            INTERNAL_TEXTURE_FORMAT);
        state.active_framebuffer = state.map_b_framebuffer;
    }
    return !current_map_is_a;
}

GLuint create_ssbo (const void *p, const GLsizeiptr size, const int flags)
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
        INTERNAL_TEXTURE_FORMAT,
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

GLuint create_framebuffer (const GLuint texture)
{
    GLuint FramebufferName = 0;
    glGenFramebuffers (1, &FramebufferName);
    glBindFramebuffer (GL_FRAMEBUFFER, FramebufferName);
    glBindTexture (GL_TEXTURE_2D, texture);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glFramebufferTexture (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

    // Set the list of draw buffers.
    constexpr GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers (1, DrawBuffers);
    return FramebufferName;
}

vertex_data_t init_vertex_data (
    const float (*vertices)[], const GLsizeiptr size, const int num)
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

void cleanup_glfw (const state_t *s)
{
    for (int i = 0; i < NUM_UNIFORMS; i++)
    {
        free (s->uniforms_buffer_object.names[i]);
    }
    LOG (LOG_INFO, "Destroying buffers\n");
    glDeleteVertexArrays (1, &s->vertex_data.VAO);
    glDeleteBuffers (1, &s->vertex_data.VBO);
    glDeleteBuffers (1, &s->uniforms_buffer_object.ubo);
    glDeleteBuffers (1, &s->ssbo);
    LOG (LOG_INFO, "Destroying framebuffers\n");
    glDeleteFramebuffers (1, &s->map_a_framebuffer);
    glDeleteFramebuffers (1, &s->map_b_framebuffer);
    LOG (LOG_INFO, "Destroying textures\n");
    glDeleteTextures (1, &s->map_a);
    glDeleteTextures (1, &s->map_b);
    free (s->uniforms_buffer_object.mem);
    LOG (LOG_INFO, "Destroying shader\n");
    glDeleteProgram (s->shader);
    LOG (LOG_INFO, "Destroying window\n");
    glfwDestroyWindow (s->window);
    LOG (LOG_INFO, "Terminating glfw\n");
    glfwTerminate ();
}

static void error_callback (const int error, const char *description)
{
    LOG (LOG_ERROR, "Error(%d): %s\n", error, description);
}

void framebuffer_size_callback (
    [[maybe_unused]] GLFWwindow *window, const int width, const int height)
{
    // make sure the viewport matches the new window dimensions; note that
    // width and height will be significantly larger than specified on retina
    // displays.
    state.size[0] = (GLfloat)width;
    state.size[1] = (GLfloat)height;
    glViewport (0, 0, width, height);
    memcpy (
        state.uniforms_buffer_object.mem, state.size, 2 * sizeof (GLfloat));
}

GLuint create_uniform_buffer (const GLuint program)
{
    glUseProgram (program);

    GLint num_blocks;
    glGetProgramiv (program, GL_ACTIVE_UNIFORM_BLOCKS, &num_blocks);
    LOG (LOG_INFO, "Num uniform blocks: %u\n", num_blocks);

    state.uniforms_buffer_object.index
        = glGetUniformBlockIndex (program, "uniforms_buffer");
    LOG (LOG_INFO, "Uniform index: %d\n", state.uniforms_buffer_object.index);

    glGetActiveUniformBlockiv (program,
        state.uniforms_buffer_object.index,
        GL_UNIFORM_BLOCK_DATA_SIZE,
        &state.uniforms_buffer_object.block_size);
    LOG (LOG_INFO,
        "Uniform size: %d\n",
        state.uniforms_buffer_object.block_size);
    state.uniforms_buffer_object.mem
        = calloc (1, state.uniforms_buffer_object.block_size);

    glGetUniformIndices (program,
        NUM_UNIFORMS,
        (const GLchar *const *)state.uniforms_buffer_object.names,
        state.uniforms_buffer_object.indices);
    glGetActiveUniformsiv (program,
        NUM_UNIFORMS,
        state.uniforms_buffer_object.indices,
        GL_UNIFORM_OFFSET,
        state.uniforms_buffer_object.offset);
    for (int i = 0; i < NUM_UNIFORMS; i++)
    {
        LOG (LOG_INFO,
            " - index %s: %d - offset: %d\n",
            state.uniforms_buffer_object.names[i],
            state.uniforms_buffer_object.indices[i],
            state.uniforms_buffer_object.offset[i]);
    }
    memcpy (state.uniforms_buffer_object.mem
                + state.uniforms_buffer_object.offset[SIZE_UNIFORM_INDEX],
        state.size,
        2 * sizeof (GLfloat));
    memcpy (state.uniforms_buffer_object.mem
                + state.uniforms_buffer_object.offset[MAP_SIZE_INDEX],
        state.map_size,
        4 * sizeof (GLfloat));
    memcpy (state.uniforms_buffer_object.mem
                + state.uniforms_buffer_object.offset[TIME_UNIFORM_INDEX],
        &state.time,
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

void print_uniforms (const GLuint program)
{
    GLint num_blocks = 0;
    glGetProgramInterfaceiv (
        program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &num_blocks);
    constexpr GLenum block_properties[1] = { GL_NUM_ACTIVE_VARIABLES };
    constexpr GLenum active_unif_prop[1] = { GL_ACTIVE_VARIABLES };
    constexpr GLenum unif_properties[3]
        = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };

    for (int blockIx = 0; blockIx < num_blocks; ++blockIx)
    {
        GLint num_active_unifs = 0;
        glGetProgramResourceiv (program,
            GL_UNIFORM_BLOCK,
            blockIx,
            1,
            block_properties,
            1,
            nullptr,
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
            nullptr,
            &block_unifs[0]);
        for (int unifIx = 0; unifIx < num_active_unifs; ++unifIx)
        {
            GLint values[3];
            glGetProgramResourceiv (program,
                GL_UNIFORM,
                block_unifs[unifIx],
                3,
                unif_properties,
                3,
                nullptr,
                values);
            state.uniforms_buffer_object.names[unifIx]
                = calloc (values[0], sizeof (char));
            glGetProgramResourceName (program,
                GL_UNIFORM,
                block_unifs[unifIx],
                values[0],
                nullptr,
                state.uniforms_buffer_object.names[unifIx]);
            LOG (LOG_INFO,
                " - Name: %s\n",
                state.uniforms_buffer_object.names[unifIx]);
        }
        free (block_unifs);
    }
}

APIENTRY void opengl_error_callback (GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *user_param)
{
    const char *source_str = nullptr;
    const char *type_str = nullptr;
    const char *severity_str = nullptr;
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        source_str = "API";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        source_str = "APPLICATION";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        source_str = "OTHER";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        source_str = "SHADER_COMPILER";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        source_str = "THIRD_PARTY";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        source_str = "WINDOW_SYSTEM";
        break;
    default:
        source_str = "UNKNOWN_SOURCE";
        break;
    }
    switch (type)
    {
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        type_str = "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_ERROR:
        type_str = "ERROR";
        break;
    case GL_DEBUG_TYPE_MARKER:
        type_str = "MARKER";
        break;
    case GL_DEBUG_TYPE_OTHER:
        type_str = "OTHER";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        type_str = "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        type_str = "POP_GROUP";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        type_str = "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        type_str = "PUSH_GROUP";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        type_str = "UNDEFINED_BEHAVIOR";
        break;
    default:
        type_str = "UNKNOWN_TYPE";
        break;
    }
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        severity_str = "HIGH";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        severity_str = "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severity_str = "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        severity_str = "NOTIFICATION";
        return;
        break;
    default:
        severity_str = "UNKNOWN_SEVERITY";
        break;
    }
    LOG (LOG_ERROR,
        "(%d) %s(%s) in %s:\n",
        id,
        type_str,
        source_str,
        severity_str);
    LOG (LOG_CONTINUE, " - %s\n", message);
}
