#pragma once

#include "../signal_buffer.h"

namespace OrangeSodium{

// Biquad filter class

class BiquadFilter {
public:
    BiquadFilter();
    ~BiquadFilter();

    enum class EFilterType {
        kLowPass = 0,
        kHighPass,
        kBandPass,
        kNotch,
        kPeak,
        kLowShelf,
        kHighShelf
    };

    void setSampleRate(float rate);
    void setFilterType(EFilterType type);
    void setCutoffFrequency(float frequency);
    void setQFactor(float q);
    void setGain(float gain_db); // For peak and shelving filters

    void calculateCoefficients();
    float tick(float input_sample);


private:
    float sample_rate;
    EFilterType filter_type;
    float cutoff_frequency;
    float q_factor;
    float gain_db;

    // Biquad coefficients
    float a0, a1, a2, b0, b1, b2;
};

}