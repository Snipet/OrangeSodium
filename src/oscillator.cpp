#include "oscillator.h"

namespace OrangeSodium {

template <typename T>
Oscillator<T>::Oscillator(Context* context, T* sample_rate)
    : pitch_hz(0), sample_rate(*sample_rate), m_context(context) {
}

// Explicit template instantiations
template class Oscillator<float>;
template class Oscillator<double>;

}
