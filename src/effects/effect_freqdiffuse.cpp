#include "effect_freqdiffuse.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace OrangeSodium {

FreqDiffuseEffect::FreqDiffuseEffect(Context* context, ObjectID id, size_t n_channels, size_t num_stages)
    : Effect(context, id, n_channels), num_stages_(num_stages) {
    effect_type = EEffectType::kFilter;

    modulation_source_names.resize(0);

    // Initialize all-pass filter coefficients and states
    a_.resize(num_stages_, 0.5f);
    z_.resize(n_channels * num_stages_, 0.0f);

    // Set up frequency-dependent dispersion (20 Hz to 20 kHz)
    setupFrequencyDispersion(20.0f, 20000.0f);

    // Add modulation source names
    modulation_source_names.push_back("diffusion_stages");
}

void FreqDiffuseEffect::setupFrequencyDispersion(float freq_min, float freq_max) {
    if (num_stages_ == 0 || sample_rate <= 0.0f) {
        return;
    }

    // Calculate logarithmically-spaced cutoff frequencies
    float log_min = std::log2(freq_min);
    float log_max = std::log2(freq_max);
    float log_step = (log_max - log_min) / static_cast<float>(num_stages_);

    for (size_t i = 0; i < num_stages_; ++i) {
        // Logarithmic frequency spacing
        float log_freq = log_min + log_step * static_cast<float>(i);
        float fc = std::pow(2.0f, log_freq);

        // Calculate all-pass coefficient for this cutoff frequency
        // H(z) = (a + z^-1) / (1 + a*z^-1)
        // where a = (tan(pi*fc/fs) - 1) / (tan(pi*fc/fs) + 1)
        float tan_val = std::tan(static_cast<float>(M_PI) * fc / sample_rate);
        float a = (tan_val - 1.0f) / (tan_val + 1.0f);

        // Clamp to stable range
        a_[i] = std::clamp(a, -0.9999f, 0.9999f);
    }
}

void FreqDiffuseEffect::processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) {
    for (size_t c = 0; c < n_channels; ++c) {
        float* in_buffer = audio_inputs->getChannel(c);
        float* out_buffer = outputs->getChannel(c);

        if (!in_buffer || !out_buffer) {
            continue;
        }

        for (size_t i = 0; i < n_audio_frames; ++i) {
            float x = in_buffer[i + frame_offset];
            // Process through all-pass stages
            for (size_t stage = 0; stage < num_stages_; ++stage) {
                size_t z_index = c * num_stages_ + stage;
                float z1 = z_[z_index];
                float a = a_[stage];
                float t = x - a * z1;
                float y1 = a * t + z1;
                z_[z_index] = t;  // Fixed: state should only store v[n], not v[n] + a*y[n]
                x = y1;
            }
            out_buffer[i + frame_offset] = x;
        }
    }
    frame_offset += n_audio_frames;
}

void FreqDiffuseEffect::onSampleRateChange(float new_sample_rate) {
    this->sample_rate = new_sample_rate;
    // Recalculate all-pass coefficients for the new sample rate
    setupFrequencyDispersion(20.0f, 20000.0f);
}
}