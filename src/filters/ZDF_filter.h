#pragma once
#include "../filter.h"

namespace OrangeSodium{

class ZDFFilter : public Filter {
public:
    ZDFFilter(Context* context, ObjectID id, size_t n_channels);
    ~ZDFFilter() override;

    void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs) override;
    void onSampleRateChange(float new_sample_rate) override;

    /// @brief Set the filter type (low-pass, high-pass, band-pass)
    /// @param type The filter type
    void setFilterType(EFilterType type);

private:

    // ZDF SVF state variables (per channel)
    // Fixed order-2 filter needs 2 integrator states per channel
    float* ic1eq;  // Integrator 1 state [channel]
    float* ic2eq;  // Integrator 2 state [channel]
    float* ic3eq;  // Integrator 3 state [channel]
    float* ic4eq;  // Integrator 4 state [channel]

    float* g_smooth; // Smoothed g values per channel
    float g_smooth_coeff;

    EFilterType filter_type;
};

}