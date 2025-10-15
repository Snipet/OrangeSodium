#include "filter.h"

namespace OrangeSodium{

template <typename T>
Filter<T>::Filter(Context* context) : m_context(context) {
    sample_rate = static_cast<T>(context->sample_rate);
    cutoff = static_cast<T>(1000.0); // Default cutoff 1kHz
    resonance = static_cast<T>(0.0); // Default resonance 0
}

// Explicit template instantiations
template class Filter<float>;
template class Filter<double>;
}