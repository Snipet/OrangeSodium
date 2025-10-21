// Abstraction of anything that produces a modulation signal
#pragma once
#include "context.h"
#include "signal_buffer.h"

namespace OrangeSodium {

class ModulationProducer {
public:
    ModulationProducer(Context* context);
    ~ModulationProducer() = default;

    /// @brief Run the modulation
    /// @param mod_inputs External modulation inputs; mod_inputs[0] is a retrigger signal; everything else is implementation specific
    /// @param outputs Audio output of modulation producer
    virtual void processBlock(SignalBuffer* mod_inputs, SignalBuffer* outputs) = 0;

protected:
    Context* m_context;
};

}