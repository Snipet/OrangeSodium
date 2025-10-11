#pragma once
#include "../effect.h"
#include "../modulation_router.h"

namespace OrangeSodium {

template <typename T>
class SimpleDistortion : public Effect<T> {
public:
    SimpleDistortion(Context* context);

    void processBlock(SignalBuffer<T>* audio_inputs, SignalBuffer<T>* mod_inputs, SignalBuffer<T>* outputs) override;
    void onSampleRateChange(T new_sample_rate) override;
    void onModulationArchitectureChange(SignalBuffer<T>* mod_inputs) override;


    static const std::map<std::string, size_t> param_map = {
        {"drive", 0},
        {"mix", 1},
        {"output_gain", 2}
    };

private:
    T param_drive = 1.0; // Drive amount
    T param_mix = 0.5;  // Dry/Wet mix
    T param_output_gain = 1.0; // Output gain

    // Internal implementation variables; These are calculated from param variables and modulators
    T impl_drive;
    T impl_mix;
    T impl_output_gain;

    std::unique_ptr<ModulationRouter<T>> mod_router;
};

}