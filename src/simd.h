// SIMD implmentation
#pragma once
#define OS_AVX

#ifdef OS_AVX
#include <immintrin.h>
#elif defined(OS_SSE)
#include <xmmintrin.h>
#include <emmintrin.h>
#else
#error "No SIMD instruction set defined"
#endif


#ifdef OS_AVX
#define OS_SIMD_WIDTH 8
typedef __m256 os_simd_t;
#define OS_SIMD_LOAD(x) _mm256_loadu_ps(x)
#define OS_SIMD_STORE(x, y) _mm256_storeu_ps(x, y)
#define OS_SIMD_SET1(x) _mm256_set1_ps(x)
#define OS_SIMD_ADD(x, y) _mm256_add_ps(x, y)
#define OS_SIMD_SUB(x, y) _mm256_sub_ps(x, y)
#define OS_SIMD_MUL(x, y) _mm256_mul_ps(x, y)
#define OS_SIMD_DIV(x, y) _mm256_div_ps(x, y)
#define OS_SIMD_MIN(x, y) _mm256_min_ps(x, y)
#define OS_SIMD_MAX(x, y) _mm256_max_ps(x, y)
#define OS_SIMD_BLEND(mask, a, b) _mm256_blendv_ps(a, b, mask)
#define OS_SIMD_CMPGT(x, y) _mm256_cmp_ps(x, y, _CMP_GT_OQ)
#define OS_SIMD_CMPLT(x, y) _mm256_cmp_ps(x, y, _CMP_LT_OQ)
#define OS_SIMD_CMPGE(x, y) _mm256_cmp_ps(x, y, _CMP_GE_OQ)
#define OS_SIMD_CMPLE(x, y) _mm256_cmp_ps(x, y, _CMP_LE_OQ)
#define OS_SIMD_CMPNEQ(x, y) _mm256_cmp_ps(x, y, _CMP_NEQ_OQ)
#define OS_SIMD_CMPEQ(x, y) _mm256_cmp_ps(x, y, _CMP_EQ_OQ)
#define OS_SIMD_HADD(x) _mm256_hadd_ps(x, x)
#define OS_SIMD_HADD2(x) _mm256_hadd_ps(x, x); _mm256_hadd_ps(x, x)
#define OS_SIMD_EXTRACT(x, idx) _mm256_extract_ps(x, idx)
#define OS_SIMD_PERMUTE(x, imm) _mm256_permute_ps(x, imm)
#define OS_SIMD_FMADD(x, y, z) _mm256_fmadd_ps(x, y, z)
#define OS_SIMD_FMSUB(x, y, z) _mm256_fmsub_ps(x, y, z)
#define OS_SIMD_FNMSUB(x, y, z) _mm256_fnmsub_ps(x, y, z)
#define OS_SIMD_FNMMADD(x, y, z) _mm256_fnmadd_ps(x, y, z)
#elif defined(OS_SSE)
#define OS_SIMD_WIDTH 4
typedef __m128 os_simd_t;
#define OS_SIMD_LOAD(x) _mm_loadu_ps(x)
#define OS_SIMD_STORE(x, y) _mm_storeu_ps(x, y)
#define OS_SIMD_SET1(x) _mm_set1_ps(x)
#define OS_SIMD_ADD(x, y) _mm_add_ps(x, y)
#define OS_SIMD_SUB(x, y) _mm_sub_ps(x, y)
#define OS_SIMD_MUL(x, y) _mm_mul_ps(x, y)
#define OS_SIMD_DIV(x, y) _mm_div_ps(x, y)
#define OS_SIMD_MIN(x, y) _mm_min_ps(x, y)
#define OS_SIMD_MAX(x, y) _mm_max_ps(x, y)
#define OS_SIMD_BLEND(mask, a, b) _mm_blendv_ps(a, b, mask)
#define OS_SIMD_CMPGT(x, y) _mm_cmpgt_ps(x, y
#define OS_SIMD_CMPLT(x, y) _mm_cmplt_ps(x, y)
#define OS_SIMD_CMPGE(x, y) _mm_cmpge_ps(x, y)
#define OS_SIMD_CMPLE(x, y) _mm_cmple_ps(x, y
#define OS_SIMD_CMPNEQ(x, y) _mm_cmpneq_ps(x, y)
#define OS_SIMD_CMPEQ(x, y) _mm_cmpeq_ps(x, y)
#define OS_SIMD_HADD(x) _mm_hadd_ps(x, x)
#define OS_SIMD_HADD2(x) _mm_hadd_ps(x, x); _mm_hadd_ps(x, x)
#define OS_SIMD_EXTRACT(x, idx) _mm_extract_ps(x, idx)
#define OS_SIMD_PERMUTE(x, imm) _mm_permute_ps(x, imm)
#define OS_SIMD_FMADD(x, y, z) _mm_fmadd_ps(x, y, z)
#define OS_SIMD_FMSUB(x, y, z) _mm_fmsub_ps(x, y, z)
#define OS_SIMD_FNMSUB(x, y, z) _mm_fnmsub_ps(x, y, z)
#define OS_SIMD_FNMMADD(x, y, z) _mm_fnmadd_ps(x, y, z)
#endif