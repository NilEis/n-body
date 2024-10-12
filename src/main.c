#include "backend.h"

static struct
{
    double dt;
    double last_frame;
} delta_time = { 0 };

extern void crash (void);
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
    while (!glfwWindowShouldClose (window))
    {
        bool speed_up = glfwGetKey (window, GLFW_KEY_SPACE) != GLFW_PRESS;
        draw ();
        for (int i = 0; i < (speed_up ? 20 : 1); i++)
        {
            update ();
        }
        glfwPollEvents ();
        if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }
    }

    backend_deinit ();
    return 0;
}