//Synthesizer implementation
#pragma once
#include "context.h"
#include "voice.h"
#include <memory>

namespace OrangeSodium {
template <typename T>
class Synthesizer {
public:
    Synthesizer(Context* context, size_t n_voices);
    ~Synthesizer();

private:
    std::vector<std::unique_ptr<Voice<T>>> voices;
    Context* m_context;
};
}