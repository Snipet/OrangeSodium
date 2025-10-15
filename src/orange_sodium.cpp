#include "orange_sodium.h"
#include "synthesizer.h"
#include "context.h"
#include <iostream>

namespace OrangeSodium{

template <typename T>
Synthesizer<T>* createSynthesizerFromScript(std::string script_path){
    Context* context = new Context();
    std::cout << "[orange_sodium.cpp] Created context" << std::endl;


    Synthesizer<T>* synth = new Synthesizer<T>(context, 16);
    std::cout << "[orange_sodium.cpp] Created synthesizer" << std::endl;

    std::cout << "[orange_sodium.cpp] Loading script: " << script_path << std::endl;
    synth->loadScript(script_path);
    std::cout << "[orange_sodium.cpp] Loaded script" << std::endl;

    return synth;
}

// Explicit template instantiations
template Synthesizer<float>* createSynthesizerFromScript<float>(std::string script_path);
template Synthesizer<double>* createSynthesizerFromScript<double>(std::string script_path);

} // namespace OrangeSodium