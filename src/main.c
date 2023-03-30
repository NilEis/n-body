#include <stdlib.h>
#include <stdio.h>
#include "particle.h"
#include "camera.h"
#include "defines.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/gl.h"
#include "ma-log.h"
#include "cglm/cglm.h"
#include "shader.h"
#include "solver.h"

#if NUM_THREADS != 0
#ifdef __STDC_NO_ATOMICS__
// Atomic :(
int sem_comp = 0;
int sem_up = 0;
#else
#include <stdatomic.h>
atomic_uint sem_comp = ATOMIC_VAR_INIT(0);
atomic_uint sem_up = ATOMIC_VAR_INIT(0);
#endif
#endif

#if NUM_THREADS != 0 && !USE_CUDA
#if (defined(__unix__) || USE_PTHREAD)
#include <pthread.h>
void *tick(void *data);
#elif !defined(__unix__)
#include <windows.h>
DWORD WINAPI tick(void *data);
#else
void tick(void);
#endif
#elif USE_CUDA

#endif
typedef struct
{
    Particle *particles;
    unsigned int start;
    unsigned int end;
} thread_arg;

Particle particles[NUM_PARTICLES] = {0};

void error_callback(int error, const char *description);

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void process_input(GLFWwindow *window);

GLuint create_shader_prog(void);

static inline void initialize_camera(void);

static camera cam = {0};

static struct
{
    double dt;
    double last_frame;
} delta_time = {0};

int main()
{
    initialize_camera();
    fill_particles_random((Particle *)&particles, NUM_PARTICLES, (vec3_t){-P_RANGE, -P_RANGE, -P_RANGE}, (vec3_t){P_RANGE, P_RANGE, P_RANGE});
    // Init GLFW
    if (!glfwInit())
    {
        LOG(LOG_ERROR, "Could not initialize glfw\n");
    }
    glfwSetErrorCallback(error_callback);
    // Create window and context
    glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
    GLFWwindow *window = glfwCreateWindow(640, 480, "N-Body", NULL, NULL);
    if (!window)
    {
        LOG(LOG_ERROR, "Could not create window or context\n");
    }

    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGL(glfwGetProcAddress);

    GLuint shader_prog = create_shader_prog();

    mat4 model_mat = {0};
    glm_mat4_identity(model_mat);
    mat4 view_mat = {0};
    mat4 proj_mat = {0};
#if NUM_THREADS != 0 && !USE_CUDA
#if (defined(__unix__) || USE_PTHREAD)
    pthread_t thread[NUM_THREADS];
#elif !defined(__unix__)
    HANDLE thread[NUM_THREADS];
#endif
    {
        unsigned int n_size = NUM_PARTICLES / NUM_THREADS;
        for (int i = 0; i < NUM_THREADS; i++)
        {
            thread_arg *t_args = (thread_arg *)calloc(sizeof(thread_arg), 1);
            t_args->particles = (Particle *)&particles;
            t_args->start = i * n_size;
            t_args->end = t_args->start + n_size;
            if (i + 1 == NUM_THREADS)
            {
                t_args->end = NUM_PARTICLES;
            }
            printf("%d <--> %d\n", t_args->start, t_args->end);
#if defined(__unix__) || USE_PTHREAD
            pthread_create(&thread[i], NULL, tick, (void *)t_args);
#elif !defined(__unix__)
            thread[i] = CreateThread(NULL, 0, tick, (void *)t_args, 0, NULL);
#endif
        }
    }
#endif
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind the vertex array object and vertex buffer object
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader_prog);

        double currentFrame = glfwGetTime();
        delta_time.dt = currentFrame - delta_time.last_frame;
        delta_time.last_frame = currentFrame;
        // printf("%f\n", delta_time.dt);
        // printf("%f,%f,%f - %f,%f,%f\n", particles[0].pos.x, particles[0].pos.y, particles[0].pos.z, particles[0].vel.x, particles[0].vel.y, particles[0].vel.z);
        process_input(window);
#if NUM_THREADS == 0 && USE_CUDA == 0
        tick();
#endif
        // fill_particles_random((Particle *)&particles, N, (vec3_t){-10.0, -10.0, -10.0}, (vec3_t){10.0, 10.0, 10.0});
        glm_lookat(cam.pos, cam.center, cam.up, view_mat);
        glm_perspective(radians(cam.fov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 10000000.0f, proj_mat);

        glUniformMatrix4fv(glGetUniformLocation(shader_prog, "model"), 1, GL_FALSE, (const GLfloat *)model_mat);
        glUniformMatrix4fv(glGetUniformLocation(shader_prog, "view"), 1, GL_FALSE, (const GLfloat *)view_mat);
        glUniformMatrix4fv(glGetUniformLocation(shader_prog, "projection"), 1, GL_FALSE, (const GLfloat *)proj_mat);
        // Copy the particle data to the buffer
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // Set the vertex attribute pointers
        glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, sizeof(Particle), (void *)0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * NUM_PARTICLES, particles, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);

        // Draw the particles as points
        glBindVertexArray(VAO);
        glPointSize(1.0f); // Set the size of the points
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
#if NUM_THREADS != 0 && !USE_CUDA
    for (int i = 0; i < NUM_THREADS; i++)
    {
#if (defined(__unix__) || USE_PTHREAD)
        pthread_cancel(thread[i]);
#elif !defined(__unix__)
        CloseHandle(thread[i]);
#endif
    }
#endif
    glDeleteProgram(shader_prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void error_callback(int error, const char *description)
{
    LOG(LOG_ERROR, "Error: %s\n", description);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    double xpos = xposIn;
    double ypos = yposIn;

    if (cam.mouse.first_mouse_event)
    {
        cam.mouse.last_x = xpos;
        cam.mouse.last_y = ypos;
        cam.mouse.first_mouse_event = 0;
    }

    double xoffset = xpos - cam.mouse.last_x;
    double yoffset = cam.mouse.last_y - ypos; // reversed since y-coordinates go from bottom to top

    cam.mouse.last_x = xpos;
    cam.mouse.last_y = ypos;

    camera_update_mouse(&cam, xoffset, yoffset);
}

void process_input(GLFWwindow *window)
{
    cam.boost = 1;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, 1);
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        cam.boost = 2;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        move_camera(&cam, CAMERA_FORWARD, delta_time.dt);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        move_camera(&cam, CAMERA_BACKWARD, delta_time.dt);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        move_camera(&cam, CAMERA_LEFT, delta_time.dt);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        move_camera(&cam, CAMERA_RIGHT, delta_time.dt);
    }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera_scroll(&cam, yoffset);
}

GLuint create_shader_prog(void)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &basic_shader_vs, NULL);
    glCompileShader(vertexShader);

    // check for vertex shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        LOG(LOG_ERROR, "Vertex shader compilation failed: %s\n", infoLog);
    }

    // compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &basic_shader_fs, NULL);
    glCompileShader(fragmentShader);

    // check for fragment shader compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        LOG(LOG_ERROR, "Fragment shader compilation failed: %s\n", infoLog);
    }

    // link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        LOG(LOG_ERROR, "Shader program linking failed: %s\n", infoLog);
    }

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

static inline void initialize_camera(void)
{
    cam.pos[0] = 0.0f;
    cam.pos[1] = 0.0f;
    cam.pos[2] = P_RANGE;
    cam.front[0] = 0.0f;
    cam.front[1] = 0.0f;
    cam.front[2] = -1.0f;
    cam.up[0] = 0.0f;
    cam.up[1] = 1.0f;
    cam.up[2] = 0.0f;
    cam.right[0] = 1.0f;
    cam.right[1] = 0.0f;
    cam.right[2] = 0.0f;
    cam.yaw = -90.0f;
    cam.pitch = 0.0f;
    cam.boost = 1;
    cam.fov = 60.0f;
    cam.mouse.last_x = 0.0f;
    cam.mouse.last_y = 0.0f;
    cam.mouse.sensitivity = 0.1f;
    cam.mouse.first_mouse_event = 1;
    cam.v = 5.0f;
    cam.Zoom = 45.0f;
}

#if !USE_CUDA
#if (defined(__unix__) || USE_PTHREAD) && NUM_THREADS != 0
void *tick(void *data)
#elif NUM_THREADS != 0
DWORD WINAPI tick(void *data)
#else
void tick(void)
#endif
{
#if NUM_THREADS != 0
    thread_arg args = {0};
    args.particles = ((thread_arg *)data)->particles;
    args.start = ((thread_arg *)data)->start;
    args.end = ((thread_arg *)data)->end;
    free(data);
#endif
#if NUM_THREADS != 0
    while (1)
    {
        while (sem_up != 0)
            ;
        ++sem_comp;
        compute_forces_newtonian(args.particles, NUM_PARTICLES, args.start, args.end);
        // compute_forces_schwarzschild_GR(particles, N, args.start, args.end);
        --sem_comp;
        while (sem_comp != 0)
            ;
        ++sem_up;
        update_particles(args.particles, NUM_PARTICLES, args.start, args.end);
        --sem_up;
    }
#else
    compute_forces_newtonian(particles, N, 0, N);
    update_particles(particles, N, 0, N);
#endif
#if NUM_THREADS != 0
    return 0;
#endif
}
#endif