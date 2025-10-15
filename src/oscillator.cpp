#include "oscillator.h"

namespace OrangeSodium {

template <typename T>
Oscillator<T>::Oscillator(Context* context, ObjectID id, size_t n_channels)
    : m_context(context), id(id), n_channels(n_channels) {
    sample_rate = static_cast<T>(context->sample_rate);
}

// Explicit template instantiations
template class Oscillator<float>;
template class Oscillator<double>;

}
