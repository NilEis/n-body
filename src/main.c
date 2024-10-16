#include "backend.h"

static struct
{
    double dt;
    double last_frame;
} delta_time = { 0 };

extern void crash(void);
int main ()
{
    if (backend_init () == -1)
    {
        return 1;
    }
    for(int i = 0; i < 0; i++)
    {
        update ();
    }
    GLFWwindow *window = backend_get_window ();
    while (!glfwWindowShouldClose (window))
    {
        draw ();
        for(int i = 0; i < 1; i++)
        {
            update ();
        }
        glfwPollEvents ();
    }

    backend_deinit ();
    return 0;
}