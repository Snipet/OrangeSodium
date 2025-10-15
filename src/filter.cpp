#include "filter.h"

namespace OrangeSodium{

template <typename T>
Filter<T>::Filter(Context* context, ObjectID id, size_t n_channels) : m_context(context), id(id) {
    sample_rate = static_cast<T>(context->sample_rate);
    param_cutoff = static_cast<T>(1000.0); // Default cutoff 1kHz
    param_resonance = static_cast<T>(0.0); // Default resonance 0
    this->n_channels = n_channels;
    this->id = id;
}

// Explicit template instantiations
template class Filter<float>;
template class Filter<double>;
}