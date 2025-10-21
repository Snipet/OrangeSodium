#include "filter.h"

namespace OrangeSodium{

Filter::Filter(Context* context, ObjectID id, size_t n_channels) : m_context(context), id(id) {
    sample_rate = static_cast<float>(context->sample_rate);
    param_cutoff = static_cast<float>(1000.0); // Default cutoff 1kHz
    param_resonance = static_cast<float>(0.0); // Default resonance 0
}