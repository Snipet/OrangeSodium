//Abstraction for effects

#pragma once
#include "context.h"
#include "signal_buffer.h"
#include <vector>
#include <string>

namespace OrangeSodium{
class Effect {
public:
    enum class EEffectType {
        kDistortion = 0,
        kFilter,
    };

    Effect(Context* context, ObjectID id, size_t n_channels);
    ~Effect() = default;

    /// @brief Run the effect
    /// @param audio_inputs Audio input to be processed
    /// @param mod_inputs External modulation inputs; implementation-specific
    /// @param outputs Audio output of effect
    virtual void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) = 0;

    /// @brief Called when the sample rate changes
    /// @param new_sample_rate The new sample rate
    virtual void onSampleRateChange(float new_sample_rate) = 0;

    /// @brief Set the sample rate and notify the effect
    /// @param rate The new sample rate
    void setSampleRate(float rate) { sample_rate = rate; onSampleRateChange(rate); }

    virtual void setInputBuffer(SignalBuffer* buffer) {
        input_buffer = buffer;
    }
    virtual void setOutputBuffer(SignalBuffer* buffer) {
        output_buffer = buffer;
    }
    virtual void setModulationBuffer(SignalBuffer* buffer) {
        mod_buffer = buffer;
    }

    virtual SignalBuffer* getInputBuffer() const {
        return input_buffer;
    }
    virtual SignalBuffer* getOutputBuffer() const {
        return output_buffer;
    }
    virtual SignalBuffer* getModulationBuffer() const {
        return mod_buffer;
    }

    EEffectType getEffectType() const {
        return effect_type;
    }

    void setEffectType(EEffectType type) {
        effect_type = type;
    }

    ObjectID getId() const {
        return id;
    }

    std::vector<std::string>& getModulationSourceNames() {
        return modulation_source_names;
    }

    virtual void beginBlock() {
        frame_offset = 0;
    }

    size_t getFrameOffset() const {
        return frame_offset;
    }

protected:
    Context* m_context;
    float sample_rate;
    size_t n_channels;
    // Names of modulation sources connected to this oscillator. This is used for linking modulation sources by name in Lua.
    std::vector<std::string> modulation_source_names;

    SignalBuffer* input_buffer;
    SignalBuffer* output_buffer;
    SignalBuffer* mod_buffer;

    EEffectType effect_type;
    ObjectID id; // Unique ID for this effect instance

    size_t frame_offset; // To allow for per-sample MIDI events, we keep track of the current frame offset within the block being processed
};
}