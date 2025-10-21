#include "oscillator.h"

namespace OrangeSodium {

Oscillator::Oscillator(Context* context, ObjectID id, size_t n_channels)
    : m_context(context), id(id), n_channels(n_channels) {
    sample_rate = static_cast<float>(context->sample_rate);
}

}
