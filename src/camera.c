#include "camera.h"
#include "ma-log.h"
#include <math.h>

void update_camera(camera *cam);

void update_camera(camera *cam)
{
    cam->front[0] = cos(radians(cam->yaw)) * cos(radians(cam->pitch));
    cam->front[1] = sin(radians(cam->pitch));
    cam->front[2] = sin(radians(cam->yaw)) * cos(radians(cam->pitch));
    glm_vec3_normalize(cam->front); //vec3_normalize(cam->front);
    vec3 tmp = {0};
    glm_vec3_cross(cam->front, (vec3){0, 1, 0}, cam->right);
    glm_vec3_normalize(cam->right); //vec3_normalize(vec3_cross(cam->front, (vec3){0, 1, 0})); // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    glm_vec3_cross(cam->right, cam->front, cam->up);
    glm_vec3_normalize(cam->up);
    glm_vec3_add(cam->pos, cam->front, cam->center);
}

void move_camera(camera *cam, camera_dir_e d, double dt)
{
    double v = cam->v * cam->boost * dt*10;
    if (d == CAMERA_FORWARD)
    {
        vec3 tmp = {0};
        glm_vec3_scale(cam->front, v, tmp);
        glm_vec3_add(tmp, cam->pos, cam->pos);
    }
    if (d == CAMERA_BACKWARD)
    {
        vec3 tmp = {0};
        glm_vec3_scale(cam->front, v, tmp);
        glm_vec3_sub(cam->pos, tmp, cam->pos);
    }
    if (d == CAMERA_LEFT)
    {
        vec3 tmp = {0};
        glm_vec3_scale(cam->right, v, tmp);
        glm_vec3_sub(cam->pos, tmp, cam->pos);
    }
    if (d == CAMERA_RIGHT)
    {
        vec3 tmp = {0};
        glm_vec3_scale(cam->right, v, tmp);
        glm_vec3_add(tmp, cam->pos, cam->pos);
    }
    update_camera(cam);
}

void camera_update_mouse(camera *cam, double offset_x, double offset_y)
{
    offset_x *= cam->mouse.sensitivity;
    offset_y *= cam->mouse.sensitivity;

    cam->yaw += offset_x;
    cam->pitch += offset_y;

    if (cam->pitch > 89.0f)
        cam->pitch = 89.0f;
    if (cam->pitch < -89.0f)
        cam->pitch = -89.0f;

    // update Front, Right and Up Vectors using the updated Euler angles
    update_camera(cam);
}

void camera_scroll(camera *cam, double offset)
{
    cam->v += offset;
    if (cam->v < 1.0f)
        cam->v = 1.0f;
}