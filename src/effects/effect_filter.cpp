#include "effect_filter.h"
#include "../filters/ZDF_filter.h"

namespace OrangeSodium {

FilterEffect::FilterEffect(Context* context, ObjectID id, size_t n_channels, Filter::EFilterObjects filter_type)
    : Effect(context, id, n_channels), filter_object_type(filter_type), filter(nullptr) {
    // Create filter instance based on specified type
    switch (filter_type) {
        case Filter::EFilterObjects::kZDF:
            filter = new ZDFFilter(context, id, n_channels);
            break;
        default:
            filter = nullptr;
            break;
    }

    effect_type = EEffectType::kFilter;

    modulation_source_names.resize(0);
    modulation_source_names.push_back("cutoff");
    modulation_source_names.push_back("resonance");
}

FilterEffect::~FilterEffect() {
    if (filter) {
        delete filter;
        filter = nullptr;
    }
}

void FilterEffect::processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) {
    if (filter) {
        filter->processBlock(audio_inputs, mod_inputs, outputs);
    }
}

void FilterEffect::onSampleRateChange(float new_sample_rate) {
    if (filter) {
        filter->setSampleRate(new_sample_rate);
    }
    this->sample_rate = new_sample_rate;
}

}