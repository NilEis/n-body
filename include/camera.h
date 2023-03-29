#ifndef CAMERA_H
#define CAMERA_H

#include "vec3.h"

typedef struct {
    vec3 pos;
    vec3 front;
    vec3 up;
    vec3 right;
    double yaw;
    double pitch;
    double fov;
} camera;

#endif // CAMERA_H
