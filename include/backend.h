#ifndef BACKEND_H
#define BACKEND_H

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

int backend_init(void);

GLFWwindow *backend_get_window(void);

void draw(void);

void backend_deinit(void);

#endif // BACKEND_H
