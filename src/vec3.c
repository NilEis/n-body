#include "vec3.h"
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "ma-log.h"

typedef uint64_t __attribute__((vector_size(4 * sizeof(uint64_t)))) vec_4i_t;

vec3_t vec3_add(vec3_t a, vec3_t b)
{
    return a + b;
}

vec3_t vec3_sub(vec3_t a, vec3_t b)
{
    return a - b;
}

vec3_m_t vec3_dot(vec3_t a, vec3_t b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

vec3_t vec3_cross(vec3_t a, vec3_t b)
{
    vec3_t l_a = __builtin_shuffle(a, (vec_4i_t){1, 2, 0, 3});
    vec3_t l_b = __builtin_shuffle(b, (vec_4i_t){2, 0, 1, 3});
    vec3_t r_a = __builtin_shuffle(a, (vec_4i_t){2, 0, 1, 3});
    vec3_t r_b = __builtin_shuffle(b, (vec_4i_t){1, 2, 0, 3});
    return (l_a * r_b) - (l_b * r_a);
}

vec3_t vec3_mul(vec3_m_t s, vec3_t v)
{
    return v * (vec3_t){s, s, s, 0};
}

vec3_t vec3_div(vec3_t v, vec3_m_t s)
{
    return v / (vec3_t){s, s, s, 0};
}

vec3_m_t vec3_length(vec3_t v)
{
    return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

vec3_t vec3_normalize(vec3_t v)
{
    vec3_m_t length = vec3_length(v);
    return vec3_div(v, length);
}

vec3_t random_vec3(vec3_m_t min_val, vec3_m_t max_val)
{
    vec3_t result;
    result[0] = min_val + ((double)rand() / RAND_MAX) * (max_val - min_val);
    result[1] = min_val + ((double)rand() / RAND_MAX) * (max_val - min_val);
    result[2] = min_val + ((double)rand() / RAND_MAX) * (max_val - min_val);
    result[3] = 0;
    return result;
}