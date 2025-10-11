//Abstraction for effects

#pragma once
#include "context.h"

namespace OrangeSodium{
template <typename T>
class Effect {
public:
    Effect(Context* context) : m_context(context), sample_rate(44100) {}
    ~Effect();

    /// @brief Run the effect
    /// @param audio_inputs Audio input to be processed
    /// @param mod_inputs External modulation inputs; implementation-specific
    /// @param outputs Audio output of effect
    virtual void processBlock(SignalBuffer<T>* audio_inputs, SignalBuffer<T>* mod_inputs, SignalBuffer<T>* outputs) = 0;

    /// @brief Called when the sample rate changes
    /// @param new_sample_rate The new sample rate
    virtual void onSampleRateChange(T new_sample_rate) = 0;

    /// @brief Get the current sample rate
    /// @return The current sample rate
    T getSampleRate() const { return sample_rate; }

    /// @brief Set the sample rate and notify the effect
    /// @param rate The new sample rate
    void setSampleRate(T rate) { sample_rate = rate; onSampleRateChange(rate); }

    /// @brief Called when the modulation architecture changes
    /// @param mod_inputs The new modulation inputs (also contains per-sample modulation buffers, but not usually needed here)
    virtual void onModulationArchitectureChange(SignalBuffer<T>* mod_inputs) = 0;

protected:
    Context* m_context;
    T sample_rate;
};
}