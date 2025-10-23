//Template oscillator class
#pragma once

#include "context.h"
#include "signal_buffer.h"
#include "utilities.h"
#include <vector>
#include <string>

namespace OrangeSodium{

class Oscillator {
public:
    enum class EModChannel{
        kPitch = 0,
        kAmplitude,
    };

    Oscillator(Context* context, ObjectID id, size_t n_channels, float amplitude = 1.0f);
    virtual ~Oscillator() = default;

    /// @brief Run the oscillator
    /// @param audio_inputs External audio source that may affect synthesis (FM, PM, etc.)
    /// @param mod_inputs External modulation inputs; mod_inputs[0] is pitch (in hz); mod_inputs[1] is amplitude [0, 1]; rest is implementation-specific
    /// @param outputs Audio output of oscillator
    virtual void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) = 0;
    virtual void onSampleRateChange(float new_sample_rate) = 0;
    void setSampleRate(float rate) { sample_rate = rate; onSampleRateChange(rate); }
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

    void resizeBuffers(size_t n_frames) {
        if (output_buffer) {
            output_buffer->resize(output_buffer->getNumChannels(), n_frames);
        }
        if (mod_buffer) {
            // Get temporary channel divisions
            size_t* channel_divisions = new size_t[mod_buffer->getNumChannels()];
            for (size_t c = 0; c < mod_buffer->getNumChannels(); ++c) {
                channel_divisions[c] = mod_buffer->getChannelDivision(c);
            }
            mod_buffer->resize(mod_buffer->getNumChannels(), n_frames);
            // Restore channel divisions
            for (size_t c = 0; c < mod_buffer->getNumChannels(); ++c) {
                mod_buffer->setChannelDivision(c, channel_divisions[c]);
            }
            delete[] channel_divisions;
        }
    }

    std::vector<std::string>& getModulationSourceNames() {
        return modulation_source_names;
    }

protected:
    Context* m_context;
    ObjectID id; // Unique ID for this oscillator instance (used for modulation routing, etc)
    float sample_rate;
    size_t n_channels = 0; // Number of output channels
    SignalBuffer* output_buffer = nullptr; // Output buffer for the oscillator
    SignalBuffer* mod_buffer = nullptr;
    EObjectType object_type;
    float amplitude; // [0, 1] Amplitude of the oscillator output
    
    // Names of modulation sources connected to this oscillator. This is used for linking modulation sources by name in Lua.
    std::vector<std::string> modulation_source_names;

    float getHzFromMIDINote(float midi_note) {
        return 440.0f * std::pow(2.0f, (midi_note - 69.f) / 12.0f);
    }
};

}