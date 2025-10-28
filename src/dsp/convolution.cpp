#include "convolution.h"
#include <cstring>  // for std::memmove

namespace OrangeSodium {

Convolver::Convolver(float* ir_buffer, size_t ir_length)
    : ir_length(ir_length) {
    impulse_response = new float[ir_length];
    for (size_t i = 0; i < ir_length; ++i) {
        impulse_response[i] = ir_buffer[i];
    }
    history_buffer = new float[ir_length];
    for (size_t i = 0; i < ir_length; ++i) {
        history_buffer[i] = 0.0f;
    }
}

Convolver::~Convolver() {
    if (impulse_response) {
        delete[] impulse_response;
        impulse_response = nullptr;
    }
    if (history_buffer) {
        delete[] history_buffer;
        history_buffer = nullptr;
    }
}

void Convolver::setImpulseResponse(float* ir_buffer, size_t ir_length) {
    if (impulse_response) {
        delete[] impulse_response;
    }
    impulse_response = new float[ir_length];
    this->ir_length = ir_length;
    for (size_t i = 0; i < ir_length; ++i) {
        impulse_response[i] = ir_buffer[i];
    }
    // Reset history buffer
    if (history_buffer) {
        delete[] history_buffer;
    }
    history_buffer = new float[ir_length];
    for (size_t i = 0; i < ir_length; ++i) {
        history_buffer[i] = 0.0f;
    }
}

float Convolver::tick(float input) {
    // Shift history buffer to make room for new input
    // Using memmove for efficient overlapping memory copy
    if (ir_length > 1) {
        std::memmove(&history_buffer[1], &history_buffer[0], (ir_length - 1) * sizeof(float));
    }
    history_buffer[0] = input;

    // Compute convolution sum using SIMD
    os_simd_t acc = OS_SIMD_SET1(0.0f);

    size_t i = 0;
    const size_t simd_end = (ir_length / OS_SIMD_WIDTH) * OS_SIMD_WIDTH;

    // Process OS_SIMD_WIDTH samples at a time using FMA
    for (; i < simd_end; i += OS_SIMD_WIDTH) {
        os_simd_t h = OS_SIMD_LOAD(&history_buffer[i]);
        os_simd_t ir = OS_SIMD_LOAD(&impulse_response[i]);
        acc = OS_SIMD_FMADD(h, ir, acc);  // acc += h * ir
    }

    // Horizontal sum of SIMD accumulator
    float sum = 0.0f;
#ifdef OS_AVX
    // For AVX: sum all 8 lanes
    // Extract high and low 128-bit lanes
    __m128 low = _mm256_castps256_ps128(acc);
    __m128 high = _mm256_extractf128_ps(acc, 1);
    __m128 sum128 = _mm_add_ps(low, high);

    // Horizontal add within 128-bit lane
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);

    // Extract scalar result
    sum = _mm_cvtss_f32(sum128);
#else
    // For SSE or other: store and manually sum
    float temp[OS_SIMD_WIDTH];
    OS_SIMD_STORE(temp, acc);
    for (int j = 0; j < OS_SIMD_WIDTH; ++j) {
        sum += temp[j];
    }
#endif

    // Handle remaining elements (tail)
    for (; i < ir_length; ++i) {
        sum += history_buffer[i] * impulse_response[i];
    }

    return sum;
}



} // namespace OrangeSodium