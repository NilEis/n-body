#ifndef OPENCL_TICK_H
#define OPENCL_TICK_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "particle.h"
#include "defines.h"
#include "ma-log.h"
#include "glad/gl.h"
#ifdef __unix__
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#else
//#warning "If it is not unix, it can only ever be windows. There are no other OSs"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

    void init_opencl_tick(Particle *p, int n, GLuint buffobj, GLFWwindow *window, volatile int* init);
    void opencl_tick(Particle *p, volatile int *running, int n, double dt, int time_steps);
    void free_opencl_tick(void);

#ifdef __cplusplus
}
#endif

#endif // OPENCL_TICK_H
