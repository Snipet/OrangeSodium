//Template filter class
#pragma once
#include "context.h"
#include "signal_buffer.h"
#include <algorithm>

#define MAX_FILTER_MOD_PARAMETERS 2 // Currently only cutoff and resonance

namespace OrangeSodium{

class Filter {
public:

    enum class EFilterType{
        kLowPass = 0,
        kHighPass,
        kBandPass
    };

    enum class EFilterObjects{
        kZDF = 0,
    };
    // enum class EProcessingMode{
    //     kPerVoice = 0, // Each voice has its own instance of the filter
    //     kParallelVoices, // This filter processes voices separately and uses SIMD to process multiple voices at once
    //     kGlobal // Master filter (processing FX, etc)
    // };

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
    void setSampleRate(float rate) { 
        sample_rate = rate; 
        max_frequency = std::min(rate / 2.f, 22050.f);
        onSampleRateChange(rate); 
    }


    float getCutoff() const { return param_cutoff; }
    float getResonance() const { return param_resonance; }

    void setCutoff(float cutoff) { param_cutoff = cutoff; }
    void setResonance(float resonance) { param_resonance = resonance; }

    static EFilterObjects getFilterObjectTypeFromString(const std::string type_str);

    static int getDefaultDivisions() { return 1; }


protected:
    Context* m_context; // Pointer to global context
    float sample_rate;
    float param_cutoff;
    float param_resonance;
    float max_frequency;
    float min_frequency = 8.f;
    ObjectID id; // Unique ID for this filter instance (used for modulation routing, etc)
    size_t n_channels;
    std::vector<std::string> modulation_source_names;

    // Convert a "knob value" in the range [0, 1] to a frequency in Hz (e.g., for cutoff control)
    float knobValueToFrequency(float value){
        float min_freq = 8.f;
        value = std::clamp(value, 0.0f, 1.0f);
        float log_min = std::log10(min_freq);
        float log_max = std::log10(max_frequency);
        float log_freq = log_min + value * (log_max - log_min);
        return std::pow(10.f, log_freq);
    }

    float frequencyToKnobValue(float frequency) {
        float min_freq = 8.f;
        frequency = std::clamp(frequency, min_freq, max_frequency);
        float log_min = std::log10(min_freq);
        float log_max = std::log10(max_frequency);
        float log_freq = std::log10(frequency);
        return (log_freq - log_min) / (log_max - log_min);
    }
};

}