#ifndef VEC3_H
#define VEC3_H
#include "defines.h"

typedef double vec3_m_t;

typedef vec3_m_t vec3_t __attribute__((vector_size(4 * sizeof(vec3_m_t))));

// Add two vectors using intrinsics
vec3_t vec3_add(vec3_t a, vec3_t b);

// Subtract two vectors using intrinsics
vec3_t vec3_sub(vec3_t a, vec3_t b);

// Compute the dot product of two vectors using intrinsics
vec3_m_t vec3_dot(vec3_t a, vec3_t b);

// Compute the cross product of two vectors using intrinsics
vec3_t vec3_cross(vec3_t a, vec3_t b);

// Multiply a vector by a scalar using intrinsics
vec3_t vec3_mul(vec3_m_t s, vec3_t v);

// Divide a vector by a scalar using intrinsics
vec3_t vec3_div(vec3_t v, vec3_m_t s);

// Compute the length of a vector using intrinsics
vec3_m_t vec3_length(vec3_t v);

// Normalize a vector using intrinsics
vec3_t vec3_normalize(vec3_t v);

vec3_t random_vec3(vec3_m_t min_val, vec3_m_t max_val);
#else

#endif // VEC3_H