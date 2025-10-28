#include "effect.h"

namespace OrangeSodium {

Effect::Effect(Context* context, ObjectID id, size_t n_channels)
    : m_context(context), n_channels(n_channels), id(id) {
    sample_rate = static_cast<float>(context->sample_rate);
}


}