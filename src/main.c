#include "backend.h"
#include "thread_creator.h"

#include <stdlib.h>

static struct
{
    double dt;
    double last_frame;
} delta_time = { 0 };

THREAD_FUNCTION (update_wrapper)
{
    _sleep (5000);
    while (1)
    {
        update ();
    }
}

int main ()
{
    if (backend_init () == -1)
    {
        return 1;
    }
    GLFWwindow *window = backend_get_window ();
    const thread t = thread_create (update_wrapper, NULL);
    while (!glfwWindowShouldClose (window))
    {
        draw ();
        glfwPollEvents ();
        if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }
    }
    thread_stop (t);

    backend_deinit ();
    return 0;
}