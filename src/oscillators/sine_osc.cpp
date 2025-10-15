#include "sine_osc.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace OrangeSodium {
template <typename T>
SineOscillator<T>::SineOscillator(Context* context, ObjectID id, size_t n_channels)
    : Oscillator<T>(context, id, n_channels) {

    }
template <typename T>
void SineOscillator<T>::processBlock(SignalBuffer<T>* audio_inputs, SignalBuffer<T>* mod_inputs, SignalBuffer<T>* outputs) {
    const size_t n_frames = outputs->getChannelLength(0); //Channel 0 is guaranteed to exist if n_channels > 0

    // Get pitch buffer from mod_inputs
    T* pitch_buffer = mod_inputs->getChannel(static_cast<size_t>(EModChannel::kPitch));
    const size_t pitch_buffer_divisions = mod_inputs->getChannelDivision(static_cast<size_t>(EModChannel::kPitch));
    for(size_t c = 0; c < n_channels; ++c) {
        T* out_buffer = outputs->getChannel(c);
        if (!out_buffer) {
            continue;
        }

        T pitch = pitch_buffer[0];
        for (size_t i = 0; i < n_frames; ++i) {
            const T pitch_norm = pitch_buffer[i / pitch_buffer_divisions] * (static_cast<T>(i) / sample_rate);
            T phase = static_cast<T>(2.0 * M_PI * pitch_norm);
            out_buffer[i] = std::sin(phase);
        }
    }
}

template <typename T>
void SineOscillator<T>::onSampleRateChange(T new_sample_rate) {
    this->sample_rate = new_sample_rate;
}
}

// Explicit template instantiations
template class OrangeSodium::SineOscillator<float>;
template class OrangeSodium::SineOscillator<double>;