#pragma once
#include "../oscillator.h"

namespace OrangeSodium {
template <typename T>
class SineOscillator : public Oscillator<T> {
public:
    SineOscillator(Context* context, ObjectID id, size_t n_channels);
    ~SineOscillator() = default;

    void processBlock(SignalBuffer<T>* audio_inputs, SignalBuffer<T>* mod_inputs, SignalBuffer<T>* outputs) override;
    void onSampleRateChange(T new_sample_rate) override;


};
}