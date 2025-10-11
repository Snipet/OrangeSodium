#include "simple_distortion.h"


namespace OrangeSodium {

template <typename T>
OrangeSodium::SimpleDistortion<T>::SimpleDistortion(Context* context)
    : Effect<T>(context), mod_router(std::make_unique<ModulationRouter<T>>(context)) {
    // Register parameter sources and implementation variables with modulation router
    mod_router->registerParamSource(&param_drive, &impl_drive);
    mod_router->registerParamSource(&param_mix, &impl_mix);
    mod_router->registerParamSource(&param_output_gain, &impl_output_gain);
}

template <typename T>
void SimpleDistortion<T>::onModulationArchitectureChange(){
    
}

}