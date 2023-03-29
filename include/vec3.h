#ifndef VEC3_H
#define VEC3_H

typedef struct vec3 {
    double x;
    double y;
    double z;
} vec3;

// Add two vectors using intrinsics
vec3 vec3_add(vec3 a, vec3 b);

// Subtract two vectors using intrinsics
vec3 vec3_sub(vec3 a, vec3 b);

// Compute the dot product of two vectors using intrinsics
double vec3_dot(vec3 a, vec3 b);

// Compute the cross product of two vectors using intrinsics
vec3 vec3_cross(vec3 a, vec3 b);

// Multiply a vector by a scalar using intrinsics
vec3 vec3_mul(double s, vec3 v);

// Divide a vector by a scalar using intrinsics
vec3 vec3_div(vec3 v, double s);

// Compute the length of a vector using intrinsics
double vec3_length(vec3 v);

// Normalize a vector using intrinsics
vec3 vec3_normalize(vec3 v);

vec3 random_vec3(double min_val, double max_val);

#endif // VEC3_H