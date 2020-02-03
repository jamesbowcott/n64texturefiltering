#include "vector_simd.h"

#include <immintrin.h>

#ifdef __AVX__
vec4 vec4muls_avx(vec4 v, double s)
{
    vec4 o;

    __m256d a = _mm256_load_pd(&v);
    __m256d b = _mm256_broadcast_sd(&s);
    __m256d c = _mm256_mul_pd(a, b);
    _mm256_store_pd(&o, c);
    
    return o;
}
#endif
