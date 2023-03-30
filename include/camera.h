#ifndef CAMERA_H
#define CAMERA_H

#include "cglm/cglm.h"

#define radians(x) ((x) * (M_PI / 180.0))

typedef enum
{
    CAMERA_FORWARD,
    CAMERA_BACKWARD,
    CAMERA_LEFT,
    CAMERA_RIGHT,
} camera_dir_e;

typedef struct
{
    vec3 pos;
    vec3 center;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 dir;
    float fov;
    double yaw;
    double pitch;
    char boost;
    struct
    {
        float last_x;
        float last_y;
        float sensitivity;
        char first_mouse_event;
        char visible;
        char visible_set;
    } mouse;
    float v;
    float Zoom;
} camera;

void move_camera(camera *cam, camera_dir_e d, double dt);
void camera_update_mouse(camera *cam, double offset_x, double offset_y);
void camera_scroll(camera *cam, double offset);


#endif // CAMERA_H
