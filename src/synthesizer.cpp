#include "synthesizer.h"
#include <iostream>
#include "console_utility.h"
#include <fstream>
#include "filters/ZDF_filter.h"
#include "console_utility.h"
#include <cassert>

namespace OrangeSodium {

Synthesizer::Synthesizer(Context* context, float sample_rate, size_t n_frames) : m_context(context) {
    m_context->oversampling = 2;
    m_context->sample_rate = sample_rate * static_cast<double>(m_context->oversampling);
    m_context->max_n_frames = n_frames * m_context->oversampling;
    master_output_buffer = nullptr;
    audio_buffers.resize(0);

    // Create Program instance
    program = new Program(m_context, this);
    if(m_context->resource_manager == nullptr){
        m_context->resource_manager = new ResourceManager();
    }

    if(m_context->waveform_fft_manager == nullptr){
        m_context->waveform_fft_manager = new FFTManager(11); //2048-point FFT
    }

    master_amplitude = 0.2f;

    // Configure master output buffer
    master_output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, n_frames, 2); // Default to stereo
    oversampled_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, n_frames * m_context->oversampling, 2);
}

Synthesizer::~Synthesizer() {
    if (program) {
        delete program;
        program = nullptr;
    }

    if (master_output_buffer) {
        delete master_output_buffer;
        master_output_buffer = nullptr;
    }

    if (oversampled_buffer) {
        delete oversampled_buffer;
        oversampled_buffer = nullptr;
    }

    for (auto* buffer : audio_buffers) {
        if (buffer) {
            delete buffer;
        }
    }
    audio_buffers.resize(0);
    audio_buffers.clear();
}

void Synthesizer::loadScript(std::string script_path) {
    *m_context->log_stream << "[synthesizer.cpp] Loading script: " << script_path << std::endl;

    if (!program) {
        *m_context->log_stream << "[synthesizer.cpp] Error: Program instance not initialized" << std::endl;
        return;
    }

    *m_context->log_stream << "[synthesizer.cpp] Loading " << script_path << std::endl;
    if (!program->loadFromFile(script_path)) {
        *m_context->log_stream << "[synthesizer.cpp] Failed to load script: " << script_path << std::endl;
    }
}

void Synthesizer::loadScriptFromString(const std::string& script_data) {
    *m_context->log_stream << "[synthesizer.cpp] Loading script from string" << std::endl;

    if (!program) {
        *m_context->log_stream << "[synthesizer.cpp] Error: Program instance not initialized" << std::endl;
        return;
    }

    if (!program->loadFromString(script_data)) {
        *m_context->log_stream << "[synthesizer.cpp] Failed to load script from string" << std::endl;
    }
}

void Synthesizer::buildSynthFromProgram() {
    *m_context->log_stream << "[synthesizer.cpp] Building synthesizer from program" << std::endl;
    if (!program) {
        return;
    }
    ConsoleUtility::logGreen(m_context->log_stream, "======= Loading program ========");
    if (!program->execute()) {
        return;
    }

    *m_context->log_stream << "[synthesizer.cpp] Calling get_num_voices()" << std::endl;
    m_context->n_voices = program->getNumVoicesDefined();
    *m_context->log_stream << "[synthesizer.cpp] Number of voices defined: " << m_context->n_voices << std::endl;

    ConsoleUtility::logGreen(m_context->log_stream, "======= Program load complete ========");

    voices.resize(0);
    for(size_t i = 0; i < m_context->n_voices; ++i){
        Voice* new_voice = program->buildVoice();
        voices.push_back(std::unique_ptr<Voice>(new_voice));
        ConsoleUtility::logGreen(m_context->log_stream, "Added voice " + std::to_string(i));
    }

    connectEffects();
}

void Synthesizer::processBlock(float** output_buffers, size_t n_channels, size_t n_frames, size_t offset) {
    // Clear buffers
    if(master_output_buffer) {
        master_output_buffer->zeroOut();
    }
    for(auto* audio_buffer : audio_buffers) {
        if(audio_buffer) {
            audio_buffer->zeroOut();
        }
    }
    
    // Process at 2x sample rate (2x frames)
    size_t oversampled_frames = n_frames * m_context->oversampling;
    
    // Process all voices at 2x sample rate
    for(auto& voice : voices) {
        //if(!voice->isPlaying()) continue;

        // This will write to the corresponding audio buffers the voice has been assigned to output to
        voice->processVoice(oversampled_frames);
    }

    // Process all effect chains
    for(auto* effect_chain : master_effect_chains) {
        if(effect_chain) {
            // For now, just zero out modulation buffers
            effect_chain->zeroOutModulationBuffers();
            effect_chain->processBlock(oversampled_frames);
        }
    }

    // For all audio buffers in audio_output_buffer_ptrs, we will combine them into oversampled_buffer

    oversampled_buffer->zeroOut();
    for(auto* audio_buffer : audio_output_buffer_ptrs) {
        for(size_t c = 0; c < n_channels; ++c) {
            float* oversampled_data = oversampled_buffer->getChannel(c);
            float* audio_buf_data = audio_buffer->getChannel(c);
            if(oversampled_data && audio_buf_data) {
                for(size_t f = 0; f < oversampled_frames; ++f) {
                    oversampled_data[f] += audio_buf_data[f];
                }
            }
        }
    }
    
    // Downsample back to original sample rate
    for(size_t c = 0; c < n_channels; ++c) {
        float* oversampled_data = oversampled_buffer->getChannel(c);
        float* output_data = output_buffers[c];
        
        if(oversampled_data && output_data) {
            // Process in blocks for HIIR
            downsamplers[c].process_block(master_output_buffer->getChannel(c), oversampled_data, n_frames);
        }
    }

    // Copy to synth output buffers
    // for(size_t c = 0; c < n_channels; ++c) {
    //     float* master_channel = master_output_buffer->getChannel(c);
    //     float* out_channel = output_buffers[c];

    //     // Debug: copy every other sample from oversampled buffer
    //     float* oversampled_channel = oversampled_buffer->getChannel(c);
    //     if(master_channel && oversampled_channel) {
    //         for(size_t f = 0; f < n_frames; ++f) {
    //             master_channel[f] = oversampled_channel[f * m_context->oversampling];
    //         }
    //     }
    // }

    // Copy to output buffers
    for(size_t c = 0; c < n_channels; ++c){
        float* master_channel = master_output_buffer->getChannel(c);
        float* out_channel = output_buffers[c];
        if(master_channel && out_channel){
            std::memcpy(out_channel + offset, master_channel, n_frames * sizeof(float));
        }
    }
}

void Synthesizer::prepare(size_t n_channels, size_t n_frames, float sample_rate){
    m_context->max_n_frames = n_frames * m_context->oversampling;
    m_context->sample_rate = sample_rate * static_cast<double>(m_context->oversampling);
    initializeOversampling(n_channels);
    for(auto& voice : voices){
        voice->resizeBuffers(m_context->max_n_frames);
        voice->setSampleRate(m_context->sample_rate);
    }
    if(master_output_buffer) {
        master_output_buffer->resize(n_channels, n_frames);
    } else {
        master_output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, n_frames, n_channels);
    }

    if(oversampled_buffer) {
        oversampled_buffer->resize(n_channels, m_context->max_n_frames);
    } else {
        oversampled_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);
    }
    

    // Resize audio buffers
    for(auto* audio_buffer : audio_buffers) {
        if(audio_buffer) {
            audio_buffer->resize(n_channels, m_context->max_n_frames);
        }
    }

    // Configure master effect chains
    for(auto* effect_chain : master_effect_chains) {
        effect_chain->resizeBuffers(m_context->max_n_frames);
        effect_chain->setSampleRate(m_context->sample_rate);
    }

}

void Synthesizer::processMidiEvent(int midi_note, bool note_on){
    bool voice_found = false;
    for(auto& voice : voices){
        voice->incrementVoiceAge();
        // if(note_on && !voice->isPlaying()){
        //     voice->activate(midi_note);
        //     voice_found = true;
        //     break; // Activate only one voice
        // } else if(!note_on && voice->isPlaying() && voice->getCurrentMIDINote() == midi_note){
        //     voice->deactivate();
        //     break; // Deactivate only one voice
        // }
        if(note_on){
            if(!voice->isPlaying()){
                voice->activate(midi_note);
                most_recent_voice = voice.get();
                voice_found = true;
                break; // Activate only one voice
            }
        } else {
            // Find and deactivate the voice playing this note
            if(voice->isPlaying() && voice->getCurrentMIDINote() == midi_note){
                voice->deactivate();
                voice_found = true;
        }
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
            most_recent_voice = oldest_voice;
        }
    }
}


double* Synthesizer::computeHIIRCoefficients() {
    // Allocate coefficient array
    double* coeffs = new double[HIIR_COEFFS];
    
    // Use HIIR's designer to calculate coefficients
    // Parameters:
    // - coeffs: output array for coefficients
    // - nbr_coefs: number of coefficients (HIIR_COEFFS)
    // - transition_bandwidth: normalized frequency (0 to 0.5)
    // - stopband_attenuation_db: stopband rejection in dB
    
    hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(
        coeffs,
        HIIR_COEFFS,
        transition_bandwidth
    );
    
    return coeffs;
}

void Synthesizer::initializeOversampling(size_t n_channels) {
    // Clear existing
    downsamplers.clear();
    
    // Compute coefficients once
    double* coeffs = computeHIIRCoefficients();
    
    // Print coefficients for debugging if needed
    *m_context->log_stream << "[Oversampling] HIIR Coefficients (" 
                           << HIIR_COEFFS << " taps, " 
                           << stopband_attenuation_db << " dB stopband):" << std::endl;
    for(int i = 0; i < HIIR_COEFFS; ++i) {
        *m_context->log_stream << "  coeff[" << i << "] = " << coeffs[i] << std::endl;
    }
    
    // Create and configure upsamplers/downsamplers for each channel
    for(size_t c = 0; c < n_channels; ++c) {
        downsamplers.emplace_back();
        
        // Set the computed coefficients
        downsamplers[c].set_coefs(coeffs);
        
        // Clear history buffers
        downsamplers[c].clear_buffers();
    }
    
    // Clean up
    delete[] coeffs;
}

ObjectID Synthesizer::addAudioBuffer(size_t n_channels) {
    // This buffer will be oversampled
    SignalBuffer* new_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);
    ObjectID new_id = m_context->getNextObjectID();
    new_buffer->setId(new_id);
    audio_buffers.push_back(new_buffer);
    return new_id;
}

SignalBuffer* Synthesizer::getAudioBufferByID(ObjectID id) {
    // Check oversampled buffers first
    for(auto* buffer : audio_buffers) {
        if(buffer && buffer->getId() == id) {
            return buffer;
        }
    }

    return nullptr; // Not found
}

ErrorCode Synthesizer::assignAudioBufferToOutput(ObjectID buffer_id) {
    SignalBuffer* buffer = getAudioBufferByID(buffer_id);
    if(!buffer) {
        return ErrorCode::kAudioBufferNotFound;
    }

    // Add to output pointers if not already present
    for(auto* existing_buffer : audio_output_buffer_ptrs) {
        if(existing_buffer && existing_buffer->getId() == buffer_id) {
            return ErrorCode::kNoError; // Already assigned
        }
    }

    audio_output_buffer_ptrs.push_back(buffer);
    ConsoleUtility::logYellow(m_context->log_stream, "Assigned audio buffer ID " + std::to_string(buffer_id) + " to output");
    return ErrorCode::kNoError;
}

EffectChainIndex Synthesizer::addEffectChain(size_t n_channels, ObjectID input_buffer_id, ObjectID output_buffer_id) {
    EffectChainIndex chain_index = m_context->getNextMasterEffectChainIndex();
    EffectChain* effect_chain = new EffectChain(m_context, n_channels, chain_index);
    SignalBuffer* input_buffer = getAudioBufferByID(input_buffer_id);
    SignalBuffer* output_buffer = getAudioBufferByID(output_buffer_id);
    effect_chain->setIO(input_buffer, output_buffer);
    master_effect_chains.push_back(effect_chain);
    return chain_index;
}

void Synthesizer::connectEffects() {
    for(auto* effect_chain : master_effect_chains) {
        if(effect_chain) {
            effect_chain->connectEffects();
        }
    }  
}


EffectChain* Synthesizer::getEffectChainByIndex(EffectChainIndex index) {
    for(auto* effect_chain : master_effect_chains) {
        if(effect_chain && effect_chain->getIndex() == index) {
            return effect_chain;
        }
    }
    return nullptr; // Not found
}

} // namespace OrangeSodium
