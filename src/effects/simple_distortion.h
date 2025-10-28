#pragma once
#include "../effect.h"
#include "../modulation_router.h"
#include <map>
#include <memory>

namespace OrangeSodium {

class SimpleDistortion : public Effect {
public:
    SimpleDistortion(Context* context, ObjectID id, size_t n_channels);

    void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) override;
    void onSampleRateChange(float new_sample_rate) override {sample_rate = new_sample_rate; }
    void setDrive(float d) { drive = d; }
    void setMix(float m) { mix = m; }
    void setOutputGain(float g) { output_gain = g; }

private:
    float drive; // Distortion drive amount
    float mix;   // Dry/Wet mix
    float output_gain; // Output gain
};

}