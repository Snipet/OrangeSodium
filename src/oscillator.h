//Template oscillator class

#include "context.h"
#include "signal_buffer.h"

namespace OrangeSodium{

template <typename T>
class Oscillator {
public:
    Oscillator(Context* context, T* sample_rate);
    ~Oscillator() = default;

    /// @brief Run the oscillator
    /// @param audio_inputs External audio source that may affect synthesis (FM, PM, etc.)
    /// @param mod_inputs External modulation inputs; mod_inputs[0] is pitch (in hz); mod_inputs[1] is amplitude [0, 1]; rest is implementation-specific
    /// @param outputs Audio output of oscillator
    virtual void processBlock(SignalBuffer<T>* audio_inputs, SignalBuffer<T>* mod_inputs, SignalBuffer<T>* outputs) = 0;
    virtual void onSampleRateChange(T new_sample_rate) = 0;

    T getPitchHz() const { return pitch_hz; }
    void setPitchHz(T pitch) { pitch_hz = pitch; }
    T getSampleRate() const { return sample_rate; }
    void setSampleRate(T rate) { sample_rate = rate; onSampleRateChange(rate); }

protected:
    T pitch_hz;
    T sample_rate;
    Context* m_context;

};

}