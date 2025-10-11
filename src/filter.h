//Template filter class
#pragma once
#include "context.h"

namespace OrangeSodium{

template <typename T>
class Filter {
public:
    Filter(Context* context);
    ~Filter() = default;

    /// @brief Run the filter
    /// @param audio_inputs Audio input to be processed
    /// @param mod_inputs External modulation inputs; mod_inputs[0] is filter cutoff [0, 1]; mod_inputs[1] is resonance [0, 1]; everything else is implementation-specific
    /// @param outputs Audio output of filter
    virtual void processBlock(SignalBuffer<T>* audio_inputs, SignalBuffer<T>* mod_inputs, SignalBuffer<T>* outputs) = 0;
    virtual void onSampleRateChange(T new_sample_rate) = 0;
    
    T getSampleRate() const { return sample_rate; }
    void setSampleRate(T rate) { sample_rate = rate; onSampleRateChange(rate); }
    T getCutoff() const { return cutoff; }
    T getResonance() const { return resonance; }

protected:
    Context* m_context;
    T sample_rate;
    T cutoff;
    T resonance;
    std::string filter_name;
};

}