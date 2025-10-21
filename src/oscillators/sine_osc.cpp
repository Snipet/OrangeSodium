#include "sine_osc.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace OrangeSodium {
SineOscillator::SineOscillator(Context* context, ObjectID id, size_t n_channels)
    : Oscillator(context, id, n_channels) {

    }
void SineOscillator::processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) {
    const size_t n_frames = n_audio_frames; // Will always be lower than or equal to context->n_frames

    // Get pitch buffer from mod_inputs
    float* pitch_buffer = mod_inputs->getChannel(static_cast<size_t>(EModChannel::kPitch));
    const size_t pitch_buffer_divisions = mod_inputs->getChannelDivision(static_cast<size_t>(EModChannel::kPitch));
    for(size_t c = 0; c < n_channels; ++c) {
        float* out_buffer = outputs->getChannel(c);
        if (!out_buffer) {
            continue;
        }

        float pitch = pitch_buffer[0];
        for (size_t i = 0; i < n_frames; ++i) {
            const float pitch_norm = pitch_buffer[i / pitch_buffer_divisions] * (static_cast<float>(i) / sample_rate);
            float phase = static_cast<float>(2.0 * M_PI * pitch_norm);
            out_buffer[i] += std::sin(phase);
        }
    }
}

void SineOscillator::onSampleRateChange(float new_sample_rate) {
    this->sample_rate = new_sample_rate;
}
}