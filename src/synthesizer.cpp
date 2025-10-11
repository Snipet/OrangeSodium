#include "synthesizer.h"

namespace OrangeSodium {

template <typename T>
Synthesizer<T>::Synthesizer(Context* context, size_t n_voices) : m_context(context) {
    voices.reserve(n_voices);
}

template <typename T>
Synthesizer<T>::~Synthesizer() {
}

// Explicit template instantiations
template class Synthesizer<float>;
template class Synthesizer<double>;

}
