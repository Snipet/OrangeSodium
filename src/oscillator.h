//Template oscillator class
#pragma once

#include "context.h"
#include "signal_buffer.h"

namespace OrangeSodium{

class Oscillator {
public:
    enum class EModChannel{
        kPitch = 0,
        kAmplitude,
    };

    Oscillator(Context* context, ObjectID id, size_t n_channels);
    virtual ~Oscillator() = default;

    /// @brief Run the oscillator
    /// @param audio_inputs External audio source that may affect synthesis (FM, PM, etc.)
    /// @param mod_inputs External modulation inputs; mod_inputs[0] is pitch (in hz); mod_inputs[1] is amplitude [0, 1]; rest is implementation-specific
    /// @param outputs Audio output of oscillator
    virtual void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) = 0;
    virtual void onSampleRateChange(float new_sample_rate) = 0;
    void setOutputBuffer(SignalBuffer* buffer) {
        output_buffer = buffer;
    }
    inline SignalBuffer* getOutputBuffer() const {
        return output_buffer;
    }
    void setModBuffer(SignalBuffer* buffer) {
        mod_buffer = buffer;
    }
    inline SignalBuffer* getModBuffer() const {
        return mod_buffer;
    }

protected:
    Context* m_context;
    ObjectID id; // Unique ID for this oscillator instance (used for modulation routing, etc)
    float sample_rate;
    size_t n_channels = 0; // Number of output channels
    SignalBuffer* output_buffer = nullptr; // Output buffer for the oscillator
    SignalBuffer* mod_buffer = nullptr;
};

}