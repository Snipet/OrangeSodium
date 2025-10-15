//Template oscillator class
#pragma once

#include "context.h"
#include "signal_buffer.h"

namespace OrangeSodium{

template <typename T>
class Oscillator {
public:
    enum class EModChannel{
        kPitch = 0,
    };

    Oscillator(Context* context, ObjectID id, size_t n_channels);
    virtual ~Oscillator() = default;

    /// @brief Run the oscillator
    /// @param audio_inputs External audio source that may affect synthesis (FM, PM, etc.)
    /// @param mod_inputs External modulation inputs; mod_inputs[0] is pitch (in hz); mod_inputs[1] is amplitude [0, 1]; rest is implementation-specific
    /// @param outputs Audio output of oscillator
    virtual void processBlock(SignalBuffer<T>* audio_inputs, SignalBuffer<T>* mod_inputs, SignalBuffer<T>* outputs) = 0;
    virtual void onSampleRateChange(T new_sample_rate) = 0;

protected:
    Context* m_context;
    ObjectID id; // Unique ID for this oscillator instance (used for modulation routing, etc)
    T sample_rate;
    size_t n_channels = 0; // Number of output channels
};

}