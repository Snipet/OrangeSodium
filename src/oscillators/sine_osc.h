#pragma once
#include "../oscillator.h"

namespace OrangeSodium {
class SineOscillator : public Oscillator {
public:
    SineOscillator(Context* context, ObjectID id, size_t n_channels, float amplitude);
    ~SineOscillator();

    void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) override;
    void onSampleRateChange(float new_sample_rate) override;

private:
    float* phase;
};
}