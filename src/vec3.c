#include "vec3.h"
#include <math.h>
#ifdef __AVX2__
#include <immintrin.h>

vec3 vec3_add(vec3 a, vec3 b)
{
    __m256d va = _mm256_load_pd(&a.x);
    __m256d vb = _mm256_load_pd(&b.x);
    __m256d vc = _mm256_add_pd(va, vb);
    vec3 c;
    _mm256_store_pd(&c.x, vc);
    return c;
}

vec3 vec3_sub(vec3 a, vec3 b)
{
    __m256d va = _mm256_load_pd(&a.x);
    __m256d vb = _mm256_load_pd(&b.x);
    __m256d vc = _mm256_sub_pd(va, vb);
    vec3 c;
    _mm256_store_pd(&c.x, vc);
    return c;
}

double vec3_dot(vec3 a, vec3 b)
{
    __m256d va = _mm256_load_pd(&a.x);
    __m256d vb = _mm256_load_pd(&b.x);
    __m256d vc = _mm256_mul_pd(va, vb);
    __m256d vd = _mm256_hadd_pd(vc, vc);
    __m128d ve = _mm_add_pd(_mm256_extractf128_pd(vd, 1), _mm256_castpd256_pd128(vd));
    double result;
    _mm_store_sd(&result, ve);
    return result;
}

vec3 vec3_cross(vec3 a, vec3 b)
{
    __m256d va = _mm256_load_pd(&a.x);
    __m256d vb = _mm256_load_pd(&b.x);
    __m256d vc = _mm256_permute4x64_pd(va, _MM_SHUFFLE(3, 0, 2, 1));
    __m256d vd = _mm256_permute4x64_pd(vb, _MM_SHUFFLE(3, 1, 0, 2));
    __m256d ve = _mm256_mul_pd(va, vd);
    __m256d vf = _mm256_mul_pd(vc, vb);
    __m256d vg = _mm256_sub_pd(ve, vf);
    vec3 c;
    _mm256_store_pd(&c.x, vg);
    return c;
}

vec3 vec3_mul(double s, vec3 v)
{
    __m256d vs = _mm256_set1_pd(s);
    __m256d vv = _mm256_load_pd(&v.x);
    __m256d vr = _mm256_mul_pd(vs, vv);
    vec3 r;
    _mm256_store_pd(&r.x, vr);
    return r;
}

vec3 vec3_div(vec3 v, double s)
{
    __m256d vs = _mm256_set1_pd(1.0 / s);
    __m256d vv = _mm256_load_pd(&v.x);
    __m256d vr = _mm256_mul_pd(vs, vv);
    vec3 r;
    _mm256_store_pd(&r.x, vr);
    return r;
}

double vec3_length(vec3 v)
{
    __m256d vv = _mm256_load_pd(&v.x);
    __m256d vr = _mm256_mul_pd(vv, vv);
    __m256d vs = _mm256_hadd_pd(vr, vr);
    __m128d vt = _mm_add_pd(_mm256_extractf128_pd(vs, 1), _mm256_castpd256_pd128(vs));
    double result;
    _mm_store_sd(&result, vt);
    return sqrt(result);
}

vec3 vec3_normalize(vec3 v)
{
    double length = vec3_length(v);
    return vec3_div(v, length);
}
#else

vec3 vec3_add(vec3 a, vec3 b)
{
    return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3 vec3_sub(vec3 a, vec3 b)
{
    return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

double vec3_dot(vec3 a, vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3_cross(vec3 a, vec3 b)
{
    return (vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

vec3 vec3_mul(double s, vec3 v)
{
    return (vec3){v.x * s, v.y * s, v.z * s};
}

vec3 vec3_div(vec3 v, double s)
{
    return (vec3){v.x / s, v.y / s, v.z / s};
}

double vec3_length(vec3 v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3 vec3_normalize(vec3 v)
{
    double length = vec3_length(v);
    return vec3_div(v, length);
}
#endif

vec3 random_vec3(double min_val, double max_val) {
    vec3 result;
    result.x = min_val + ((double) rand() / RAND_MAX) * (max_val - min_val);
    result.y = min_val + ((double) rand() / RAND_MAX) * (max_val - min_val);
    result.z = min_val + ((double) rand() / RAND_MAX) * (max_val - min_val);
    return result;
}