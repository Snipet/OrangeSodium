#include "biquad.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace OrangeSodium {

BiquadFilter::BiquadFilter()
    : sample_rate(44100.0f),
      filter_type(EFilterType::kLowPass),
      cutoff_frequency(1000.0f),
      q_factor(0.707f),
      gain_db(0.0f),
      a0(1.0f), a1(0.0f), a2(0.0f),
      b0(1.0f), b1(0.0f), b2(0.0f) {
    calculateCoefficients();
}

BiquadFilter::~BiquadFilter() {
    // No dynamic memory to clean up
}

void BiquadFilter::setSampleRate(float rate) {
    sample_rate = rate;
    calculateCoefficients();
}

void BiquadFilter::setFilterType(EFilterType type) {
    filter_type = type;
    calculateCoefficients();
}

void BiquadFilter::setCutoffFrequency(float frequency) {
    cutoff_frequency = frequency;
    calculateCoefficients();
}

void BiquadFilter::setQFactor(float q) {
    q_factor = q;
    calculateCoefficients();
}

void BiquadFilter::setGain(float gain) {
    gain_db = gain;
    calculateCoefficients();
}

void BiquadFilter::calculateCoefficients() {
    // Precompute common values
    const float w0 = 2.0f * static_cast<float>(M_PI) * cutoff_frequency / sample_rate;
    const float cos_w0 = std::cos(w0);
    const float sin_w0 = std::sin(w0);
    const float alpha = sin_w0 / (2.0f * q_factor);

    // For peak and shelving filters
    const float A = std::pow(10.0f, gain_db / 40.0f); // sqrt of linear gain

    // Calculate coefficients based on filter type
    // Using the Audio EQ Cookbook formulas
    switch (filter_type) {
        case EFilterType::kLowPass:
            b0 = (1.0f - cos_w0) / 2.0f;
            b1 = 1.0f - cos_w0;
            b2 = (1.0f - cos_w0) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
            break;

        case EFilterType::kHighPass:
            b0 = (1.0f + cos_w0) / 2.0f;
            b1 = -(1.0f + cos_w0);
            b2 = (1.0f + cos_w0) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
            break;

        case EFilterType::kBandPass:
            // Constant 0 dB peak gain
            b0 = alpha;
            b1 = 0.0f;
            b2 = -alpha;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
            break;

        case EFilterType::kNotch:
            b0 = 1.0f;
            b1 = -2.0f * cos_w0;
            b2 = 1.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
            break;

        case EFilterType::kPeak:
            b0 = 1.0f + alpha * A;
            b1 = -2.0f * cos_w0;
            b2 = 1.0f - alpha * A;
            a0 = 1.0f + alpha / A;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha / A;
            break;

        case EFilterType::kLowShelf:
            {
                const float sqrt_A = std::sqrt(A);
                const float beta = std::sqrt(A) / q_factor;

                b0 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + beta * sin_w0);
                b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0);
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - beta * sin_w0);
                a0 = (A + 1.0f) + (A - 1.0f) * cos_w0 + beta * sin_w0;
                a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0);
                a2 = (A + 1.0f) + (A - 1.0f) * cos_w0 - beta * sin_w0;
            }
            break;

        case EFilterType::kHighShelf:
            {
                const float sqrt_A = std::sqrt(A);
                const float beta = std::sqrt(A) / q_factor;

                b0 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 + beta * sin_w0);
                b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_w0);
                b2 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 - beta * sin_w0);
                a0 = (A + 1.0f) - (A - 1.0f) * cos_w0 + beta * sin_w0;
                a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_w0);
                a2 = (A + 1.0f) - (A - 1.0f) * cos_w0 - beta * sin_w0;
            }
            break;

        default:
            // Default to pass-through
            b0 = 1.0f;
            b1 = 0.0f;
            b2 = 0.0f;
            a0 = 1.0f;
            a1 = 0.0f;
            a2 = 0.0f;
            break;
    }

    // Normalize coefficients by a0
    const float a0_inv = 1.0f / a0;
    b0 *= a0_inv;
    b1 *= a0_inv;
    b2 *= a0_inv;
    a1 *= a0_inv;
    a2 *= a0_inv;
    a0 = 1.0f;
}


float BiquadFilter::tick(float input_sample) {
    static float z1 = 0.0f; // Delay buffer 1
    static float z2 = 0.0f; // Delay buffer 2

    // Direct Form I implementation
    float output_sample = b0 * input_sample + z1;
    z1 = b1 * input_sample - a1 * output_sample + z2;
    z2 = b2 * input_sample - a2 * output_sample;

    return output_sample;

} // namespace OrangeSodium