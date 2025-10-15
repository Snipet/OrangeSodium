#include "voice.h"
namespace OrangeSodium{

template <typename T>
Voice<T>::Voice(Context* context) : m_context(context)
{

}

template <typename T>
Voice<T>::~Voice()
{
    // Clean up oscillators
    for (auto* osc : oscillators) {
        delete osc;
    }
    // Clean up filters
    for (auto* filter : filters) {
        delete filter;
    }
    // Clean up buffers
    for (auto* buffer : buffers) {
        delete buffer;
    }
}

// Explicit template instantiations
template class Voice<float>;
template class Voice<double>;

}