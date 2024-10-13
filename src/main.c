#include "backend.h"
#include "thread_creator.h"

static struct
{
    double dt;
    double last_frame;
} delta_time = { 0 };

THREAD_FUNCTION (update_wrapper)
{
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
    for (int i = 0; i < 0; i++)
    {
        update ();
    }
    GLFWwindow *window = backend_get_window ();
    thread t = thread_create (update_wrapper, NULL);
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