// Synthesizer voice
#pragma once
#include <vector>
#include "filter.h"
#include "oscillator.h"
#include "signal_buffer.h"
#include "context.h"

namespace OrangeSodium{

template <typename T>
class Voice {
public:
    Voice(Context* context);
    ~Voice();

private:
    /// @brief Collection of all voice oscillators
    std::vector<Oscillator> oscillators;

    /// @brief Collection of all voice filters
    std::vector<Filter> filters;

    /// @brief Collection of interconnecting signal buffers
    std::vector<SignalBuffer> buffers;

    Context* m_context;
};

}