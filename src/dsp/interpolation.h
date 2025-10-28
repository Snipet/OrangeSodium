#pragma once

namespace OrangeSodium {

/// @brief Linear interpolation between two samples
/// @param x0 Previous sample
/// @param x1 Next sample
/// @param frac Fractional position [0, 1] between x0 and x1
/// @return Interpolated value
inline float interpolateLinear(float x0, float x1, float frac) {
    return x0 + frac * (x1 - x0);
}

/// @brief Cubic (4-point) interpolation using Hermite splines
/// @param xm1 Sample at position -1
/// @param x0 Sample at position 0
/// @param x1 Sample at position 1
/// @param x2 Sample at position 2
/// @param frac Fractional position [0, 1] between x0 and x1
/// @return Interpolated value
inline float interpolateCubic(float xm1, float x0, float x1, float x2, float frac) {
    // 4-point, 3rd-order Hermite (x-form)
    const float c0 = x0;
    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

/// @brief Catmull-Rom spline interpolation (4-point)
/// @param xm1 Sample at position -1
/// @param x0 Sample at position 0
/// @param x1 Sample at position 1
/// @param x2 Sample at position 2
/// @param frac Fractional position [0, 1] between x0 and x1
/// @return Interpolated value
inline float interpolateCatmullRom(float xm1, float x0, float x1, float x2, float frac) {
    const float frac2 = frac * frac;
    const float frac3 = frac2 * frac;

    const float a0 = -0.5f * xm1 + 1.5f * x0 - 1.5f * x1 + 0.5f * x2;
    const float a1 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float a2 = -0.5f * xm1 + 0.5f * x1;
    const float a3 = x0;

    return a0 * frac3 + a1 * frac2 + a2 * frac + a3;
}

/// @brief 6-point, 5th-order Hermite interpolation (optimal)
/// @param buffer Pointer to buffer containing at least 6 samples starting at offset-2
/// @param frac Fractional position [0, 1] between sample[2] and sample[3]
/// @return Interpolated value
inline float interpolateHermite6(const float* buffer, float frac) {
    // Using optimal 6-point, 5th-order Hermite interpolation
    // buffer[0] = x[-2], buffer[1] = x[-1], buffer[2] = x[0],
    // buffer[3] = x[1], buffer[4] = x[2], buffer[5] = x[3]

    const float even1 = buffer[3] + buffer[2];
    const float odd1 = buffer[3] - buffer[2];
    const float even2 = buffer[4] + buffer[1];
    const float odd2 = buffer[4] - buffer[1];
    const float even3 = buffer[5] + buffer[0];
    const float odd3 = buffer[5] - buffer[0];

    const float c0 = buffer[2];
    const float c1 = 0.04166666666666666667f * odd3 - 0.29166666666666666667f * odd2 + 1.25f * odd1;
    const float c2 = 0.04166666666666666667f * even3 - 0.41666666666666666667f * even2 + 1.375f * even1 - 1.0f * buffer[2];
    const float c3 = 0.20833333333333333333f * odd2 - 0.625f * odd1 - 0.04166666666666666667f * odd3;
    const float c4 = 0.125f * even3 - 0.625f * even2 + 1.5f * even1 - buffer[2];
    const float c5 = 0.04166666666666666667f * odd3 - 0.20833333333333333333f * odd2 + 0.375f * odd1;

    return ((((c5 * frac + c4) * frac + c3) * frac + c2) * frac + c1) * frac + c0;
}

/// @brief Read from circular buffer with linear interpolation
/// @param buffer Circular buffer
/// @param buffer_size Size of the circular buffer
/// @param read_pos Fractional read position (can be >= buffer_size or negative)
/// @return Interpolated sample
inline float readCircularBufferLinear(const float* buffer, size_t buffer_size, float read_pos) {
    // Wrap read position to [0, buffer_size)
    while (read_pos < 0.0f) {
        read_pos += static_cast<float>(buffer_size);
    }
    while (read_pos >= static_cast<float>(buffer_size)) {
        read_pos -= static_cast<float>(buffer_size);
    }

    const size_t index0 = static_cast<size_t>(read_pos);
    const size_t index1 = (index0 + 1) % buffer_size;
    const float frac = read_pos - static_cast<float>(index0);

    return interpolateLinear(buffer[index0], buffer[index1], frac);
}

/// @brief Read from circular buffer with cubic interpolation
/// @param buffer Circular buffer
/// @param buffer_size Size of the circular buffer
/// @param read_pos Fractional read position (can be >= buffer_size or negative)
/// @return Interpolated sample
inline float readCircularBufferCubic(const float* buffer, size_t buffer_size, float read_pos) {
    // Wrap read position to [0, buffer_size)
    while (read_pos < 0.0f) {
        read_pos += static_cast<float>(buffer_size);
    }
    while (read_pos >= static_cast<float>(buffer_size)) {
        read_pos -= static_cast<float>(buffer_size);
    }

    const size_t index0 = static_cast<size_t>(read_pos);
    const size_t indexm1 = (index0 == 0) ? buffer_size - 1 : index0 - 1;
    const size_t index1 = (index0 + 1) % buffer_size;
    const size_t index2 = (index0 + 2) % buffer_size;
    const float frac = read_pos - static_cast<float>(index0);

    return interpolateCubic(buffer[indexm1], buffer[index0], buffer[index1], buffer[index2], frac);
}

} // namespace OrangeSodium
