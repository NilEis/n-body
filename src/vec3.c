#include "vec3.h"
#include <stdlib.h>
#include <math.h>
#include "ma-log.h"

vec3_t vec3_add(vec3_t a, vec3_t b)
{
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3_t vec3_sub(vec3_t a, vec3_t b)
{
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3_m_t vec3_dot(vec3_t a, vec3_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3_t vec3_cross(vec3_t a, vec3_t b)
{
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

vec3_t vec3_mul(vec3_m_t s, vec3_t v)
{
    return (vec3_t){v.x * s, v.y * s, v.z * s};
}

vec3_t vec3_div(vec3_t v, vec3_m_t s)
{
    return (vec3_t){v.x / s, v.y / s, v.z / s};
}

vec3_m_t vec3_length(vec3_t v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3_t vec3_normalize(vec3_t v)
{
    vec3_m_t length = vec3_length(v);
    return vec3_div(v, length);
}

vec3_t random_vec3(vec3_m_t min_val, vec3_m_t max_val) {
    vec3_t result;
    result.x = min_val + ((double) rand() / RAND_MAX) * (max_val - min_val);
    result.y = min_val + ((double) rand() / RAND_MAX) * (max_val - min_val);
    result.z = min_val + ((double) rand() / RAND_MAX) * (max_val - min_val);
    return result;
}