#include "backend.h"

static struct
{
    double dt;
    double last_frame;
} delta_time = { 0 };

int main ()
{
    if (backend_init () == -1)
    {
        return 1;
    }
    GLFWwindow *window = backend_get_window ();
    while (!glfwWindowShouldClose (window))
    {
        draw ();
        update();
        glfwPollEvents ();
    }

    backend_deinit ();
    return 0;
}