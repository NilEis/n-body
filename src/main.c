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

#if USE_CUDA
#include "cuda_tick.h"
#elif USE_OPENCL
#include "opencl_tick.h"
#else
#if defined(__unix__) && !USE_PTHREAD
#error "PTHREAD is needed"
#endif
#include "solver.h"
#endif

volatile int paused = 0;
volatile int running = 0;
volatile int tick_finished = 0;
volatile int time_steps = 1;

#ifdef __STDC_NO_ATOMICS__
// Atomic :(
volatile int sem_comp = 0;
volatile int sem_up = 0;
#else
#include <stdatomic.h>
atomic_uint sem_comp = 0;
atomic_uint sem_up = 0;
#endif

#if (defined(__unix__) || USE_PTHREAD)
#include <pthread.h>
void *dispatch_threads(void *);
#else
#include <windows.h>
DWORD WINAPI dispatch_threads(void *data);
#endif

#if NUM_THREADS != 0 && !USE_CUDA && !USE_OPENCL
#if (defined(__unix__) || USE_PTHREAD)
void *tick(void *data);
#elif !defined(__unix__)
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

Particle *particles = NULL;

void error_callback(int error, const char *description);

GLFWwindow *initGLFW(void);

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void process_input(GLFWwindow *window);

static void print_particles(void);

GLuint create_shader_prog(void);

GLuint create_compute_shader_prog(const char *src);

static inline void initialize_camera(void);

static camera cam = {0};

static int num_particles = 1500;
static double dt = 0.01;

static int window_width = 1200;
static int window_height = 800;

static struct
{
    double dt;
    double last_frame;
} delta_time = {0};

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        num_particles = atoi(argv[1]);
    }
    LOG(LOG_INFO, "Initialize camera\n");
    initialize_camera();
    LOG(LOG_INFO, "Create Particles\n");
    particles = (Particle *)calloc(num_particles, sizeof(Particle));
    LOG(LOG_INFO, "Allocated %d bytes\n", num_particles * sizeof(Particle));
    fill_particles_random(particles, num_particles, (vec3_t){-P_RANGE, -P_RANGE, -P_RANGE}, (vec3_t){P_RANGE, P_RANGE, P_RANGE});

    GLFWwindow *window = initGLFW();

    LOG(LOG_INFO, "Init GLAD\n");
    gladLoadGL(glfwGetProcAddress);

    LOG(LOG_INFO, "Load shader\n");
    GLuint shader_prog = create_shader_prog();

    mat4 model_mat = {0};
    glm_mat4_identity(model_mat);
    mat4 view_mat = {0};
    mat4 proj_mat = {0};

    LOG(LOG_INFO, "Generate buffer\n");
    GLuint VBO, VAO;
    // Gen vertex array
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    // Gen buffer
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Set buffer data
    glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * num_particles, particles, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, sizeof(Particle), (void *)0);

#if !USE_COMPUTE_SHADER
    LOG(LOG_INFO, "Dispatch tick thread\n");
    // print_particles();
#if (defined(__unix__) || USE_PTHREAD)
    pthread_t thread;
    pthread_create(&thread, NULL, dispatch_threads, NULL);
#elif !defined(__unix__)
    HANDLE thread;
    thread = CreateThread(NULL, 0, dispatch_threads, NULL, 0, NULL);
#endif
#else
    LOG(LOG_INFO, "Load solve compute shader\n");
    GLuint shader_solve_prog = create_compute_shader_prog(shader_solve_comp);
    LOG(LOG_INFO, "Load update compute shader\n");
    GLuint shader_update_prog = create_compute_shader_prog(shader_update_comp);
    GLuint shader_solve_prog_dt = glGetUniformLocation(shader_solve_prog, "dt");
    GLuint shader_update_prog_dt = glGetUniformLocation(shader_update_prog, "dt");
    GLuint dispatch_size = num_particles % 128 == 0 ? num_particles / 128 : (num_particles / 128) + 1;
    GLuint SSBO;
    glGenBuffers(1, &SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * num_particles, particles, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    LOG(LOG_INFO, "Dispatch_size: %d\n", dispatch_size);
#endif

    LOG(LOG_INFO, "Start main loop\n");
    // Bind the vertex array object and vertex buffer object
    while (!tick_finished)
    {
        if (glfwWindowShouldClose(window))
        {
#if USE_COMPUTE_SHADER
            break;
#endif
            static int stop_called = 1;
            if (stop_called)
            {
                running = 0;
                LOG(LOG_INFO, "Waiting for tick thread\n");
                stop_called = 0;
            }
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double currentFrame = glfwGetTime();
        delta_time.dt = currentFrame - delta_time.last_frame;
        delta_time.last_frame = currentFrame;
        process_input(window);
        glm_lookat(cam.pos, cam.center, cam.up, view_mat);
        glm_perspective(radians(cam.fov), (float)window_width / (float)window_height, 0.1f, 10000000.0f, proj_mat);

        glUniformMatrix4fv(glGetUniformLocation(shader_prog, "model"), 1, GL_FALSE, (const GLfloat *)model_mat);
        glUniformMatrix4fv(glGetUniformLocation(shader_prog, "view"), 1, GL_FALSE, (const GLfloat *)view_mat);
        glUniformMatrix4fv(glGetUniformLocation(shader_prog, "projection"), 1, GL_FALSE, (const GLfloat *)proj_mat);

#if USE_COMPUTE_SHADER
        Particle part;

        glUseProgram(shader_solve_prog);
        // glBindVertexArray(VAO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
        glUniform1d(shader_solve_prog_dt, dt);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, VBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBO);
        glDispatchCompute(dispatch_size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glUseProgram(shader_update_prog);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, VBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, VBO);
        glUniform1d(shader_update_prog_dt, dt);
        glDispatchCompute(dispatch_size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Particle), &part);
        //LOG(LOG_INFO, "Update: %f, %f, %f - %f, %f, %f\n", part.pos.x, part.pos.y, part.pos.z, part.vel.x, part.vel.y, part.vel.z);
#else
        glBindVertexArray(VAO);
#endif

        // glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * num_particles, particles, GL_DYNAMIC_DRAW);
        glUseProgram(shader_prog);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        //glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Particle), &part);
        //LOG(LOG_INFO, "Update VBO: %f, %f, %f - %f, %f, %f\n", part.pos.x, part.pos.y, part.pos.z, part.vel.x, part.vel.y, part.vel.z);
        //glBindVertexArray(VAO);
        glEnableVertexAttribArray(0);
        glPointSize(1.0f); // Set the size of the points
        glDrawArrays(GL_POINTS, 0, num_particles);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    LOG(LOG_INFO, "Stopped main loop\n");
#if !USE_COMPUTE_SHADER
#if (defined(__unix__) || USE_PTHREAD)
    pthread_join(thread, NULL);
#else
    WaitForSingleObject(thread, INFINITE);
#endif
    LOG(LOG_INFO, "Tick thread stopped\n");
#endif
    glDeleteProgram(shader_prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    free(particles);
    LOG(LOG_INFO, "Stopping\n");
    return 0;
}

GLuint create_compute_shader_prog(const char *src)
{
    GLuint computeShader, program;
    GLint success;
    GLchar infoLog[512];

    // create shader object
    computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &src, NULL);
    glCompileShader(computeShader);

    // check for shader compile errors
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
        LOG(LOG_ERROR, "Shader compilation failed: %s\n", infoLog);
        return 0;
    }

    // create program object
    program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);

    // check for program linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        LOG(LOG_ERROR, "Program linking failed: %s\n", infoLog);
        return 0;
    }

    // clean up
    glDeleteShader(computeShader);

    return program;
}

GLFWwindow *initGLFW(void)
{
    // Init GLFW
    LOG(LOG_INFO, "Init GLFW\n");
    if (!glfwInit())
    {
        LOG(LOG_ERROR, "Could not initialize glfw\n");
        return NULL;
    }
    glfwSetErrorCallback(error_callback);
    // Create window and context
    glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
    GLFWwindow *window = glfwCreateWindow(window_width, window_height, "N-Body", NULL, NULL);
    if (!window)
    {
        LOG(LOG_ERROR, "Could not create window or context\n");
        return NULL;
    }

    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    return window;
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
    static int p_last_press = 0;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, 1);
    }

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        print_particles();
    }

    p_last_press = p_last_press == 0 ? 0 : p_last_press - 1;

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        if (p_last_press == 0)
        {
            paused = !paused;
            p_last_press = 10;
        }
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
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
    {
        time_steps++;
        LOG(LOG_INFO, "%d\n", time_steps);
    }
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
    {
        time_steps = time_steps == 1 ? 1 : time_steps - 1;
        LOG(LOG_INFO, "%d\n", time_steps);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
        if (!cam.mouse.visible)
        {
            cam.mouse.visible = 1;
            cam.mouse.visible_set = 0;
        }
    }
    else
    {
        if (cam.mouse.visible)
        {
            cam.mouse.visible = 0;
            cam.mouse.visible_set = 0;
        }
    }
    if (!cam.mouse.visible_set)
    {
        if (cam.mouse.visible)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cam.mouse.visible = 1;
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            cam.mouse.visible = 0;
        }
        cam.mouse.visible_set = 1;
    }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera_scroll(&cam, yoffset);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
}

static void print_particles(void)
{
    for (int i = 0; i < num_particles; i++)
    {
        LOG(LOG_INFO, "Particle %d: mass: %f - x: %f, y: %f, z: %f - vx: %f, vy: %f, vz: %f\n", i, particles[i].mass, particles[i].pos.x, particles[i].pos.y, particles[i].pos.z, particles[i].vel.x, particles[i].vel.y, particles[i].vel.z);
    }
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
    cam.pos[2] = P_RANGE * 2.0;
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
    cam.v = P_RANGE / 10.0;
    cam.Zoom = 45.0f;
}

#if (defined(__unix__) || USE_PTHREAD)
void *dispatch_threads(void *data)
#else
DWORD WINAPI dispatch_threads(void *data)
#endif
{
#if NUM_THREADS != 0 && !USE_CUDA && !USE_OPENCL
#if (defined(__unix__) || USE_PTHREAD)
    pthread_t thread[NUM_THREADS];
#elif !defined(__unix__)
    HANDLE thread[NUM_THREADS];
#endif
    {
        unsigned int n_size = num_particles / NUM_THREADS;
        for (int i = 0; i < NUM_THREADS; i++)
        {
            thread_arg *t_args = (thread_arg *)calloc(sizeof(thread_arg), 1);
            t_args->particles = (Particle *)particles;
            t_args->start = i * n_size;
            t_args->end = t_args->start + n_size;
            if (i + 1 == NUM_THREADS)
            {
                t_args->end = num_particles;
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
#if USE_CUDA
    init_cuda_tick(particles, num_particles);
#elif USE_OPENCL
    init_opencl_tick(particles, num_particles);
#endif
    running = 1;
    while (running == 1)
    {
#if NUM_THREADS == 0 && !USE_CUDA && !USE_OPENCL
        if (!paused)
        {
            compute_forces_newtonian(particles, num_particles, 0, num_particles, dt);
            update_particles(particles, num_particles, 0, num_particles, dt);
        }
#elif USE_CUDA
        if (!paused)
        {
            cuda_tick(particles, (volatile int *)&running, num_particles, dt);
        }
#elif USE_OPENCL
        if (!paused)
        {
            opencl_tick(particles, (volatile int *)&running, num_particles, dt, time_steps);
        }
#else
        __asm("nop");
#endif
    }
#if USE_CUDA
    free_cuda_tick();
#elif USE_OPENCL
    free_opencl_tick();
#endif
#if NUM_THREADS != 0 && !USE_CUDA && !USE_OPENCL
    for (int i = 0; i < NUM_THREADS; i++)
    {
#if (defined(__unix__) || USE_PTHREAD)
        pthread_cancel(thread[i]);
#elif !defined(__unix__)
        CloseHandle(thread[i]);
#endif
    }
#endif
    tick_finished = 1;
#if (defined(__unix__) || USE_PTHREAD)
    return NULL;
#elif NUM_THREADS != 0
    return 0;
#endif
}

#if !USE_CUDA && !USE_OPENCL && !USE_COMPUTE_SHADER
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
        compute_forces_newtonian(args.particles, num_particles, args.start, args.end, dt);
        // compute_forces_schwarzschild_GR(particles, N, args.start, args.end);
        --sem_comp;
        while (sem_comp != 0)
            ;
        ++sem_up;
        update_particles(args.particles, num_particles, args.start, args.end, dt);
        --sem_up;
    }
#endif
#if (defined(__unix__) || USE_PTHREAD)
    return NULL;
#elif NUM_THREADS != 0
    return 0;
#endif
}
#endif