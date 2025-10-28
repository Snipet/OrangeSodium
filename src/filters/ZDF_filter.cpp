#include "ZDF_filter.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace OrangeSodium {

ZDFFilter::ZDFFilter(Context* context, ObjectID id, size_t n_channels)
    : Filter(context, id, n_channels) {
    // Initialize state variables for each channel
    // ZDF SVF requires two integrator states per channel (fixed order-2)
    ic1eq = new float[n_channels];
    ic2eq = new float[n_channels];
    ic3eq = new float[n_channels];
    ic4eq = new float[n_channels];

    for (size_t c = 0; c < n_channels; ++c) {
        ic1eq[c] = 0.0f;
        ic2eq[c] = 0.0f;
        ic3eq[c] = 0.0f;
        ic4eq[c] = 0.0f;
    }

    // Initialize filter parameters
    param_cutoff = 1000.0f;  // Default cutoff frequency in Hz
    param_resonance = 0.5f;  // Default resonance [0, 1]
    filter_type = EFilterType::kLowPass;

    modulation_source_names.resize(0);
    modulation_source_names.push_back("cutoff");
    modulation_source_names.push_back("resonance");
}

void ZDFFilter::processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs) {
    const size_t n_frames = audio_inputs->getChannelLength(0);

    // Get modulation buffers
    // mod_inputs[0] = cutoff [0, 1]
    // mod_inputs[1] = resonance [0, 1]
    float* cutoff_buffer = mod_inputs->getChannel(0);
    float* resonance_buffer = mod_inputs->getChannel(1);
    const size_t cutoff_divisions = mod_inputs->getChannelDivision(0);
    const size_t resonance_divisions = mod_inputs->getChannelDivision(1);

    for (size_t c = 0; c < n_channels; ++c) {
        float* in_buffer = audio_inputs->getChannel(c);
        float* out_buffer = outputs->getChannel(c);

        if (!in_buffer || !out_buffer) {
            continue;
        }

        for (size_t i = 0; i < n_frames; ++i) {
            // Get modulated parameters
            
            float cutoff_mod = (cutoff_buffer) ? cutoff_buffer[i / cutoff_divisions] : 0.5f;
            // Scale cutoff_mod to match exponential frequency response
            float param_cutoff_knob_val = frequencyToKnobValue(param_cutoff);
            cutoff_mod = knobValueToFrequency(param_cutoff_knob_val + cutoff_mod) / max_frequency;


            const float cutoff_hz = cutoff_mod * (max_frequency);

            // Resonance: map [0, 1] to resonance coefficient
            const float resonance_mod = (resonance_buffer) ? resonance_buffer[i / resonance_divisions] : 0.f;
            const float resonance = std::clamp(resonance_mod + param_resonance, 0.0f, 1.0f);

            // Compute ZDF SVF coefficients
            // g = tan(pi * fc / fs)
            const float g = std::tan(static_cast<float>(M_PI) * std::clamp(cutoff_hz / sample_rate, 0.0f, 0.499f));

            const float k = 0.4f;

            // ZDF SVF processing (Topology-Preserving Transform)
            const float input_signal = in_buffer[i] - k * ic4eq[c];

            float v0 = (input_signal * g + ic1eq[c]) / (1.0f + g);
            ic1eq[c] = 2.f * v0 - ic1eq[c];
            float v1 = (v0 * g + ic2eq[c]) / (1.0f + g);
            ic2eq[c] = 2.f * v1 - ic2eq[c];
            float v2 = (v1 * g + ic3eq[c]) / (1.0f + g);
            ic3eq[c] = 2.f * v2 - ic3eq[c];
            float v3 = (v2 * g + ic4eq[c]) / (1.0f + g);
            ic4eq[c] = 2.f * v3 - ic4eq[c];

            // Select output based on filter type
            float output_signal = v3;

            out_buffer[i] = output_signal;
        }
    }
}

void ZDFFilter::onSampleRateChange(float new_sample_rate) {
    sample_rate = new_sample_rate;
}

void ZDFFilter::setFilterType(EFilterType type) {
    filter_type = type;
}

ZDFFilter::~ZDFFilter() {
    delete[] ic1eq;
    delete[] ic2eq;
}

} // namespace OrangeSodium
