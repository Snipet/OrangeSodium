// Class for efficiently routing modulation signals (from mod_input) to correct destinations

/*
Orange Sodium supports per-sample modulation of parameters. This is achieved by using modulation buffers that are routed from
modulation producers to modulation destinations (e.g. oscillator pitch, filter cutoff, effect parameters, etc).
Instead of using a large, unified modulation matrix, Orange Sodium uses a more efficient approach where
each object (oscillator, filter, effect, etc) reads directly from the modulation buffers it needs.

In the Lua script, each object is assigned a unique 32-bit id.
*/

#pragma once
#include "context.h"
#include "modulation.h"
#include <vector>

namespace OrangeSodium{

class ModulationRouter {
public:
    ModulationRouter(Context* context);
    ~ModulationRouter();

    void registerParamSource(float* param_source, float* impl_source);
    void addModulation(const Modulation& modulation);


private:
    std::vector<float*> param_source; // Pointers to parameter sources (knobs). These are defined per-implementation for each object (e.g. drive for distortion effect)
    std::vector<float*> impl_source; // Pointer to variable used in DSP code. These are the results of applying modulation to param_source
    std::vector<Modulation> m_modulations;
    Context* m_context;
};

}