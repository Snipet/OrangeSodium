#include "oscillator.h"

namespace OrangeSodium {

Oscillator::Oscillator(Context* context, ObjectID id, size_t n_channels, float amplitude)
    : m_context(context), id(id), n_channels(n_channels), amplitude(amplitude) {
    sample_rate = static_cast<float>(context->sample_rate);
    object_type = EObjectType::kOscillator;
}

}
