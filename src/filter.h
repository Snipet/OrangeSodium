//Template filter class
#pragma once
#include "context.h"
#include "signal_buffer.h"

namespace OrangeSodium{

class Filter {
public:

    enum class EProcessingMode{
        kPerVoice = 0, // Each voice has its own instance of the filter
        kParallelVoices, // This filter processes voices separately and uses SIMD to process multiple voices at once
        kGlobal // Master filter (processing FX, etc)
    };

    Filter(Context* context, ObjectID id, size_t n_channels);
    virtual ~Filter() = default;

    /// @brief Run the filter
    /// @param audio_inputs Audio input to be processed
    /// @param mod_inputs External modulation inputs; mod_inputs[0] is filter cutoff [0, 1]; mod_inputs[1] is resonance [0, 1]; everything else is implementation-specific
    /// @param outputs Audio output of filter
    virtual void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs) = 0;
    virtual void onSampleRateChange(float new_sample_rate) = 0;

    /// @brief Get the current sample rate of the filter
    /// @return The current sample rate
    float getSampleRate() const { return sample_rate; }

    /// @brief Set the sample rate of the filter
    /// @param rate The new sample rate
    void setSampleRate(float rate) { sample_rate = rate; onSampleRateChange(rate); }


    float getCutoff() const { return param_cutoff; }
    float getResonance() const { return param_resonance; }

protected:
    Context* m_context; // Pointer to global context
    float sample_rate;
    float param_cutoff;
    float param_resonance;
    ObjectID id; // Unique ID for this filter instance (used for modulation routing, etc)
};

}