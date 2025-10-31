#include "orange_sodium.h"
#include "synthesizer.h"
#include "context.h"
#include <iostream>

namespace OrangeSodium{


Synthesizer* createSynthesizerFromScript(std::string script_path){


    Synthesizer* synth = new Synthesizer();
    Context* context = synth->getContext();
    *context->log_stream << "[orange_sodium.cpp] Created synthesizer" << std::endl;

    *context->log_stream << "[orange_sodium.cpp] Loading script: " << script_path << std::endl;
    synth->loadScript(script_path);
    *context->log_stream << "[orange_sodium.cpp] Loaded script" << std::endl;

    return synth;
}

Synthesizer* createSynthesizerFromString(const std::string& script_data){

    Synthesizer* synth = new Synthesizer();
    Context* context = synth->getContext();
    *context->log_stream << "[orange_sodium.cpp] Created synthesizer" << std::endl;

    *context->log_stream << "[orange_sodium.cpp] Loading script from string" << std::endl;
    synth->loadScriptFromString(script_data);
    *context->log_stream << "[orange_sodium.cpp] Loaded script from string" << std::endl;

    return synth;
}
} // namespace OrangeSodium