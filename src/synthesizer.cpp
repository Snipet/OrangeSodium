#include "synthesizer.h"
#include <iostream>
#include "console_utility.h"

namespace OrangeSodium {

template <typename T>
Synthesizer<T>::Synthesizer(Context* context, size_t n_voices, float sample_rate) : m_context(context) {
    voices.reserve(n_voices);
    m_context->n_voices = n_voices;
    m_context->sample_rate = sample_rate;

    // Get or create singleton instance
    program = Program<T>::getInstance(m_context);
}

template <typename T>
Synthesizer<T>::~Synthesizer() {
    // Note: Don't delete program here as it's a singleton
    // Call Program<T>::destroyInstance() explicitly when done with all synthesizers
}

template <typename T>
void Synthesizer<T>::loadScript(std::string script_path) {
    std::cout << "[synthesizer.cpp] Loading script: " << script_path << std::endl;

    if (!program) {
        program = Program<T>::getInstance(m_context);
    }

    std::cout << "[synthesizer.cpp] Using singleton program instance. Loading " << script_path << std::endl;
    if (!program->loadFromFile(script_path)) {
        std::cerr << "[synthesizer.cpp] Failed to load script: " << script_path << std::endl;
    }
}

template <typename T>
void Synthesizer<T>::buildSynthFromProgram() {
    std::cout << "[synthesizer.cpp] Building synthesizer from program" << std::endl;
    if (!program) {
        return;
    }
    ConsoleUtility::logGreen("======= Executing program ========");
    Voice<T>* template_voice = new Voice<T>(m_context);
    program->setTemplateVoice(template_voice);
    if (!program->execute()) {
        return;
    }

    //This is the template voice that will be cloned for each voice


    ConsoleUtility::logGreen("======= Program execution complete ========");

    std::cout << "[synthesizer.cpp] Copying template voice" << std::endl;


    //Now that the template voice is built, clone it for each voice
    // IMPORTANT!! ObjectIDs must be unique across all objects in the synthesizer
    // This means that when cloning objects, we must assign new IDs
    for (size_t i = 0; i < m_context->n_voices; ++i) {
        voices.push_back(std::make_unique<Voice<T>>(*template_voice));
    }

    std::cout << "[synthesizer.cpp] Built " << voices.size() << " voices" << std::endl;
}

// Explicit template instantiations
template class Synthesizer<float>;
template class Synthesizer<double>;

} // namespace OrangeSodium
