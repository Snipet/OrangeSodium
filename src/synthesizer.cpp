#include "synthesizer.h"
#include <iostream>
#include "console_utility.h"

namespace OrangeSodium {

template <typename T>
Synthesizer<T>::Synthesizer(Context* context, size_t n_voices) : m_context(context) {
    voices.reserve(n_voices);
    program = nullptr;
}

template <typename T>
Synthesizer<T>::~Synthesizer() {
}

template <typename T>
void Synthesizer<T>::loadScript(std::string script_path) {
    std::cout << "[synthesizer.cpp] Loading script: " << script_path << std::endl;
    if (program) {
        std::cout << "[synthesizer.cpp] Deleting existing program" << std::endl;
        delete program;
    }
    program = new Program(m_context);
    std::cout << "[synthesizer.cpp] Created program. Loading  " << script_path << std::endl;
    if (!program->loadFromFile(script_path)) {
        std::cerr << "[synthesizer.cpp] Failed to load script: " << script_path << std::endl;
        delete program;
        program = nullptr;
    }
}

template <typename T>
void Synthesizer<T>::buildSynthFromProgram() {
    std::cout << "[synthesizer.cpp] Building synthesizer from program" << std::endl;
    if (!program) {
        return;
    }

    ConsoleUtility::logGreen("======= Executing program ========");

    if (!program->execute()) {
        return;
    }


    ConsoleUtility::logGreen("======= Program execution complete ========");
}

// Explicit template instantiations
template class Synthesizer<float>;
template class Synthesizer<double>;

} // namespace OrangeSodium
