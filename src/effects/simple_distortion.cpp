#include "simple_distortion.h"


namespace OrangeSodium {

SimpleDistortion::SimpleDistortion(Context* context)
    : Effect(context), mod_router(std::make_unique<ModulationRouter>(context)) {
    // Register parameter sources and implementation variables with modulation router
    mod_router->registerParamSource(&param_drive, &impl_drive);
    mod_router->registerParamSource(&param_mix, &impl_mix);
    mod_router->registerParamSource(&param_output_gain, &impl_output_gain);
}

void SimpleDistortion::onModulationArchitectureChange(SignalBuffer* mod_inputs){
    // Update modulation router with new architecture
}

}