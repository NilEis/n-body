#ifndef STATE_H
#define STATE_H
#include "arena.h"
#include "quad.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/gl.h"

#define SIZE_ELEM 2

#define POS_ARRAY_LENGTH (SIZE_ELEM * NUM_ANTS)

typedef struct
{
    int pos_index;
    double vx;
    double vy;
    double fx;
    double fy;
    double w;
} ant;

typedef struct
{
    GLuint VBO;
    GLuint VAO;
} vertex_data_t;

typedef struct
{
    Arena arena;
    quad main_quad;
    GLFWwindow *window;
    char window_name[28];
    GLuint shader;
    GLuint comp_shader_blur;
    GLuint shader_map;
    GLuint comp_shader_sub;
    struct
    {
        struct nk_context *ctx;
    } nuklear;
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
    GLfloat ants_pos_a[POS_ARRAY_LENGTH];
    GLfloat ants_pos_b[POS_ARRAY_LENGTH];
    GLfloat (*ants_pos_read)[POS_ARRAY_LENGTH];
    GLfloat (*ants_pos_write)[POS_ARRAY_LENGTH];
    ant ants[NUM_ANTS];
    bool swapping_buffers;
    bool ant_buffer_ready;
} state_t;

#endif // STATE_H
