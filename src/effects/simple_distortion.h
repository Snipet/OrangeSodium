#pragma once
#include "../effect.h"
#include "../modulation_router.h"
#include <map>
#include <memory>

namespace OrangeSodium {

class SimpleDistortion : public Effect {
public:
    SimpleDistortion(Context* context);

    void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs) override;
    void onSampleRateChange(float new_sample_rate) override;
    void onModulationArchitectureChange(SignalBuffer* mod_inputs) override;


    inline static const std::map<std::string, size_t> param_map = {
        {"drive", 0},
        {"mix", 1},
        {"output_gain", 2}
    };

private:
    float param_drive = 1.0; // Drive amount
    float param_mix = 0.5;  // Dry/Wet mix
    float param_output_gain = 1.0; // Output gain

    // Internal implementation variables; These are calculated from param variables and modulators
    float impl_drive;
    float impl_mix;
    float impl_output_gain;

    std::unique_ptr<ModulationRouter> mod_router;
};

}