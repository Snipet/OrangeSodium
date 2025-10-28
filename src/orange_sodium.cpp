#include "orange_sodium.h"
#include "synthesizer.h"
#include "context.h"
#include <iostream>

#define NUM_VOICES_DEFAULT 8

namespace OrangeSodium{


Synthesizer* createSynthesizerFromScript(std::string script_path){
    Context* context = new Context();
    *context->log_stream << "[orange_sodium.cpp] Created context" << std::endl;


    Synthesizer* synth = new Synthesizer(context, NUM_VOICES_DEFAULT);
    *context->log_stream << "[orange_sodium.cpp] Created synthesizer" << std::endl;

    *context->log_stream << "[orange_sodium.cpp] Loading script: " << script_path << std::endl;
    synth->loadScript(script_path);
    *context->log_stream << "[orange_sodium.cpp] Loaded script" << std::endl;

    return synth;
}

Synthesizer* createSynthesizerFromString(const std::string& script_data){
    Context* context = new Context();
    *context->log_stream << "[orange_sodium.cpp] Created context" << std::endl;

    Synthesizer* synth = new Synthesizer(context, NUM_VOICES_DEFAULT);
    *context->log_stream << "[orange_sodium.cpp] Created synthesizer" << std::endl;

    *context->log_stream << "[orange_sodium.cpp] Loading script from string" << std::endl;
    synth->loadScriptFromString(script_data);
    *context->log_stream << "[orange_sodium.cpp] Loaded script from string" << std::endl;

    return synth;
}
} // namespace OrangeSodium