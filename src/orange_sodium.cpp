#include "orange_sodium.h"
#include "synthesizer.h"
#include "context.h"
#include <iostream>

namespace OrangeSodium{


Synthesizer* createSynthesizerFromScript(std::string script_path){
    Context* context = new Context();
    std::cout << "[orange_sodium.cpp] Created context" << std::endl;


    Synthesizer* synth = new Synthesizer(context, 1);
    std::cout << "[orange_sodium.cpp] Created synthesizer" << std::endl;

    std::cout << "[orange_sodium.cpp] Loading script: " << script_path << std::endl;
    synth->loadScript(script_path);
    std::cout << "[orange_sodium.cpp] Loaded script" << std::endl;

    return synth;
}


} // namespace OrangeSodium