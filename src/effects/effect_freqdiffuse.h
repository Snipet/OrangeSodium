#pragma once
#include "../effect.h"
#include <vector>
#include <algorithm>

namespace OrangeSodium {

class FreqDiffuseEffect : public Effect {
public:
    FreqDiffuseEffect(Context* context, ObjectID id, size_t n_channels, size_t num_stages);
    ~FreqDiffuseEffect() = default;

    void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) override;
    void onSampleRateChange(float new_sample_rate) override;
    void setGeometricA(float a0, float r) {
        a0 = std::clamp(a0, 0.0f, 0.9999f);
        r  = std::clamp(r,  0.0f, 0.9999f);
        float val = a0;
        for (int i = 0; i < num_stages_; ++i) {
            a_[i] = std::clamp(val, 0.0f, 0.9999f);
            val *= r;
        }
    }

    // Set up frequency-dependent all-pass filters for dispersion
    // freq_min, freq_max: frequency range for dispersion (Hz)
    void setupFrequencyDispersion(float freq_min, float freq_max);

    static size_t getMaxModulationChannels() { return 1; } // diffusion_stages

private:
    std::vector<float> a_; // All-pass filter coefficients
    std::vector<float> z_; // All-pass filter states, channels are blocked sequentially
    size_t num_stages_;
};

}