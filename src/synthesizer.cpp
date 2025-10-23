#include "synthesizer.h"
#include <iostream>
#include "console_utility.h"
#include <fstream>

namespace OrangeSodium {

Synthesizer::Synthesizer(Context* context, size_t n_voices, float sample_rate, size_t n_frames) : m_context(context) {
    voices.reserve(n_voices);
    m_context->n_voices = n_voices;
    m_context->sample_rate = sample_rate;
    m_context->max_n_frames = n_frames;
    master_output_buffer = nullptr;

    // Create Program instance
    program = new Program(m_context);
    setMasterOutputBufferInfoCallback = [this](size_t n_channels) {
        if (master_output_buffer) {
            delete master_output_buffer;
        }
        master_output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);
    };
    program->setMasterOutputBufferInfoCallback(setMasterOutputBufferInfoCallback);

}

Synthesizer::~Synthesizer() {
    if (program) {
        delete program;
        program = nullptr;
    }
}

void Synthesizer::loadScript(std::string script_path) {
    std::cout << "[synthesizer.cpp] Loading script: " << script_path << std::endl;

    if (!program) {
        std::cerr << "[synthesizer.cpp] Error: Program instance not initialized" << std::endl;
        return;
    }

    std::cout << "[synthesizer.cpp] Loading " << script_path << std::endl;
    if (!program->loadFromFile(script_path)) {
        std::cerr << "[synthesizer.cpp] Failed to load script: " << script_path << std::endl;
    }
}

void Synthesizer::buildSynthFromProgram() {
    std::cout << "[synthesizer.cpp] Building synthesizer from program" << std::endl;
    if (!program) {
        return;
    }
    ConsoleUtility::logGreen("======= Executing program ========");
    Voice* template_voice = new Voice(m_context);
    program->setTemplateVoice(template_voice);
    if (!program->execute()) {
        return;
    }

    //This is the template voice that will be cloned for each voice


    ConsoleUtility::logGreen("======= Program execution complete ========");

    //std::cout << "[synthesizer.cpp] Copying template voice" << std::endl;


    //Now that the template voice is built, clone it for each voice
    // IMPORTANT!! ObjectIDs must be unique across all objects in the synthesizer
    // This means that when cloning objects, we must assign new IDs
    // for (size_t i = 0; i < m_context->n_voices; ++i) {
    //     voices.push_back(std::make_unique<Voice>(*template_voice));
    // }

    // std::cout << "[synthesizer.cpp] Built " << voices.size() << " voices" << std::endl;

    //For now, just use the template voice as the only voice
    voices.push_back(std::unique_ptr<Voice>(template_voice));
}

void Synthesizer::processBlock(size_t n_audio_frames){
    for(auto& voice : voices){
        voice->processVoice(n_audio_frames);
    }

    // Add all voice outputs to master output buffer
    for(auto& voice : voices){
        if(!voice->isPlaying()){
            continue;
        }
        SignalBuffer* voice_buffer = voice->getMasterAudioBuffer();
        if(voice_buffer && master_output_buffer){
            for(size_t c = 0; c < master_output_buffer->getNumChannels(); ++c){
                float* master_channel = master_output_buffer->getChannel(c);
                float* voice_channel = voice_buffer->getChannel(c % voice_buffer->getNumChannels());
                if(master_channel && voice_channel){
                    for(size_t f = 0; f < n_audio_frames; ++f){
                        master_channel[f] += voice_channel[f];
                    }
                }
            }
        }
    }
}

void Synthesizer::getOutput(float** output_buffers, size_t n_channels, size_t n_frames){
    if(!master_output_buffer){
        return;
    }

    for(size_t c = 0; c < n_channels; ++c){
        float* master_channel = master_output_buffer->getChannel(c);
        float* out_channel = output_buffers[c];
        if(master_channel && out_channel){
            std::memcpy(out_channel, master_channel, n_frames * sizeof(float));
        }
    }
}

void Synthesizer::processBlock(float** output_buffers, size_t n_channels, size_t n_frames){
    // Clear master output buffer
    if(master_output_buffer){
        master_output_buffer->zeroOut();
    }

    // Process audio block
    processBlock(n_frames);

    // Get output
    getOutput(output_buffers, n_channels, n_frames);
}

void Synthesizer::prepare(size_t n_channels, size_t n_frames, float sample_rate){
    m_context->max_n_frames = n_frames;
    m_context->sample_rate = sample_rate;
    for(auto& voice : voices){
        voice->setMasterAudioBufferInfo(n_channels);
        voice->resizeBuffers(n_frames);
        voice->setSampleRate(sample_rate);
    }
    setMasterOutputBufferInfoCallback(n_channels);
    if(master_output_buffer) {
        master_output_buffer->resize(n_channels, n_frames);
    } else {
        master_output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, n_frames, n_channels);
    }
    //master_output_buffer->resize(n_channels, n_frames);

}

void Synthesizer::processMidiEvent(int midi_note, bool note_on){
    bool voice_found = false;
    for(auto& voice : voices){
        if(note_on && !voice->isPlaying()){
            voice->activate(midi_note);
            voice_found = true;
            break; // Activate only one voice
        } else if(!note_on && voice->isPlaying() && voice->getCurrentMIDINote() == midi_note){
            voice->deactivate();
            break; // Deactivate only one voice
        }
    }
    if(!voice_found && note_on){
        //Override oldest voice
        unsigned int oldest_age = 0;
        Voice* oldest_voice = nullptr;
        for(auto& voice : voices){
            if(voice->getVoiceAge() >= oldest_age){
                oldest_age = voice->getVoiceAge();
                oldest_voice = voice.get();
            }
        }
        if(oldest_voice){
            oldest_voice->activate(midi_note);
        }
    }
}

} // namespace OrangeSodium
