#include "filter.h"

namespace OrangeSodium{

Filter::Filter(Context* context, ObjectID id, size_t n_channels) : m_context(context), id(id), n_channels(n_channels) {
    sample_rate = static_cast<float>(context->sample_rate);
    param_cutoff = static_cast<float>(1000.0); // Default cutoff 1kHz
    param_resonance = static_cast<float>(0.0); // Default resonance 0
    min_frequency = 8.f;
}

Filter::EFilterObjects Filter::getFilterObjectTypeFromString(const std::string type_str) {
    if (type_str == "ZDF") {
        return EFilterObjects::kZDF;
    } 
    // Add more filter types as needed

    // Default to ZDF if unknown
    return EFilterObjects::kZDF;
}

} // namespace OrangeSodium