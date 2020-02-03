
#include "vector.h"

#include <immintrin.h>


double vec2dot(vec2 a, vec2 b)
{
    double sum = 0;
    sum += a.x * b.x;
    sum += a.y * b.y;
    
    return sum;
}

vec2 vec2sub(vec2 a, vec2 b)
{
    vec2 c;
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    return c;
}

vec2 vec2add(vec2 a, vec2 b)
{
    vec2 c;
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    return c;
}

vec4 vec4muls(vec4 v, double s)
{
#if defined(__AVX__)
    __m256d a = _mm256_load_pd(&v);
    __m256d b = _mm256_broadcast_sd(&s);
    __m256d c = _mm256_mul_pd(a, b);
    _mm256_store_pd(&v, c);
    return v;
#else
    v.w *= s;
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return v;
#endif
}

vec2 vec2muls(vec2 v, double s)
{
#if defined(__SSE2__)
    __m128d _v = _mm_loadr_pd(&v);
    __m128d _s = _mm_load1_pd(&s);
    __m128d _r = _mm_mul_pd(_v, _s);
    _mm_store_pd(&v, _r);
    return v;
#else
    v.x *= s;
    v.y *= s;
    return v;
#endif
}

vec4 vec4add(vec4 a, vec4 b)
{
#if defined(__AVX__)
    __m256d _a = _mm256_load_pd(&a);
    __m256d _b = _mm256_load_pd(&b);
    __m256d _c = _mm256_add_pd(_a, _b);
    _mm256_store_pd(&a, _c);
    return a;
#else
    a.w += b.w;
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
#endif
}

vec2 vec2make(double x, double y)
{
    vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

vec4 vec4make(double w, double x, double y, double z)
{
    vec4 v;
    v.w = w;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}
