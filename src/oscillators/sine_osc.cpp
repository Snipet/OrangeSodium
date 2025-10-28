#include "sine_osc.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace OrangeSodium {
SineOscillator::SineOscillator(Context* context, ObjectID id, size_t n_channels, float amplitude)
    : Oscillator(context, id, n_channels, amplitude) {
    phase = new float[n_channels];
    for (size_t c = 0; c < n_channels; ++c) {
        phase[c] = 0.0f;
    }

    // Add modulation source names
    modulation_source_names.resize(0);
    modulation_source_names.push_back("pitch");
    modulation_source_names.push_back("amplitude");

}


void SineOscillator::processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) {
    const size_t n_frames = n_audio_frames; // Will always be lower than or equal to context->max_n_frames

    // Get pitch buffer from mod_inputs
    float* pitch_buffer = mod_inputs->getChannel(static_cast<size_t>(EModChannel::kPitch));
    float* amplitude_buffer = mod_inputs->getChannel(static_cast<size_t>(EModChannel::kAmplitude));
    const size_t pitch_buffer_divisions = mod_inputs->getChannelDivision(static_cast<size_t>(EModChannel::kPitch));
    const size_t amplitude_buffer_divisions = mod_inputs->getChannelDivision(static_cast<size_t>(EModChannel::kAmplitude));
    for(size_t c = 0; c < n_channels; ++c) {
        float* out_buffer = outputs->getChannel(c);
        if (!out_buffer) {
            continue;
        }

        for (size_t i = 0; i < n_frames; ++i) {
            const float pitch_hz = getHzFromMIDINote(pitch_buffer[i / pitch_buffer_divisions] + frequency_offset);
            const float pitch_norm = pitch_hz / (sample_rate);
            float phase_increment = 2.0f * static_cast<float>(M_PI) * pitch_norm;
            phase[c] += phase_increment;
            phase[c] = std::fmod(phase[c], 2.0f * static_cast<float>(M_PI));
            const float amp = amplitude + ((amplitude_buffer) ? amplitude_buffer[i / amplitude_buffer_divisions] : 0.0f);
            out_buffer[i] += amp * std::sin(phase[c]);
        }
    }
}

void SineOscillator::onSampleRateChange(float new_sample_rate) {
    this->sample_rate = new_sample_rate;
}
}