#include "effect_distortion.h"
#include <algorithm>
#include <cmath>

namespace OrangeSodium {

DistortionEffect::DistortionEffect(Context* context, ObjectID id, size_t n_channels)
    : Effect(context, id, n_channels) {
    
    modulation_source_names.resize(0);
    modulation_source_names.push_back("drive");
    modulation_source_names.push_back("mix");
    modulation_source_names.push_back("output_gain");

    // Initialize parameters
    drive = 1.0f;       // Default drive
    mix = 1.0f;         // Default dry/wet mix
    output_gain = 1.0f; // Default output gain
    effect_type = EEffectType::kDistortion;
}

void DistortionEffect::processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) {

    for(size_t c = 0; c < n_channels; ++c) {
        float* in_buffer = audio_inputs->getChannel(c);
        float* out_buffer = outputs->getChannel(c);

        if (!in_buffer || !out_buffer) {
            continue; // Skip if input or output buffer is null
        }

        for (size_t i = 0; i < n_audio_frames; ++i) {
            // Apply distortion effect
            float driven_sample = in_buffer[i + frame_offset] * drive;

            // Simple tanh distortion
            float distorted_sample = std::tanh(driven_sample);

            // Mix dry and wet signals
            out_buffer[i + frame_offset] = (1.0f - mix) * in_buffer[i + frame_offset] + mix * distorted_sample;

            // Apply output gain
            out_buffer[i + frame_offset] *= output_gain;
        }
    }
    frame_offset += n_audio_frames;
}

DistortionEffect::EDistortionType DistortionEffect::getDistortionTypeFromString(const std::string& type_string) {
    std::string type_lowercase = type_string;
    std::transform(type_lowercase.begin(), type_lowercase.end(), type_lowercase.begin(), ::tolower);
    if (type_string == "tanh" || type_string == "soft") {
        return EDistortionType::kTanh;
    }
    // Default to tanh if unknown
    return EDistortionType::kTanh;
}

}