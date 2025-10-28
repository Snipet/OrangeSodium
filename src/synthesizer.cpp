#include "synthesizer.h"
#include <iostream>
#include "console_utility.h"
#include <fstream>
#include "filters/ZDF_filter.h"

namespace OrangeSodium {

Synthesizer::Synthesizer(Context* context, size_t n_voices, float sample_rate, size_t n_frames) : m_context(context) {
    voices.reserve(n_voices);
    m_context->oversampling = 2;
    m_context->n_voices = n_voices;
    m_context->sample_rate = sample_rate * static_cast<double>(m_context->oversampling);
    m_context->max_n_frames = n_frames * m_context->oversampling;
    master_output_buffer = nullptr;
    oversampled_buffer = nullptr;

    // Create Program instance
    program = new Program(m_context);
    if(m_context->resource_manager == nullptr){
        m_context->resource_manager = new ResourceManager();
    }

    if(m_context->waveform_fft_manager == nullptr){
        m_context->waveform_fft_manager = new FFTManager(11); //2048-point FFT
    }

    setMasterOutputBufferInfoCallback = [this](size_t n_channels) {
        if (master_output_buffer) {
            delete master_output_buffer;
        }
        master_output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);
    };
    program->setMasterOutputBufferInfoCallback(setMasterOutputBufferInfoCallback);

    master_amplitude = 0.3f;

}

Synthesizer::~Synthesizer() {
    if (program) {
        delete program;
        program = nullptr;
    }
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
    ConsoleUtility::logGreen(m_context->log_stream, "======= Executing program ========");
    if (!program->execute()) {
        return;
    }



    ConsoleUtility::logGreen(m_context->log_stream, "======= Program load complete ========");

    for(size_t i = 0; i < m_context->n_voices; ++i){
        Voice* new_voice = program->buildVoice();
        voices.push_back(std::unique_ptr<Voice>(new_voice));
    }
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

    // Apply master amplitude
    for(size_t c = 0; c < master_output_buffer->getNumChannels(); ++c){
        float* master_channel = master_output_buffer->getChannel(c);
        if(master_channel){
            for(size_t f = 0; f < n_audio_frames; ++f){
                master_channel[f] *= master_amplitude;
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

// void Synthesizer::processBlock(float** output_buffers, size_t n_channels, size_t n_frames){
//     // Clear master output buffer
//     n_frames *= m_context->oversampling;
//     if(master_output_buffer){
//         master_output_buffer->zeroOut();
//     }

//     // Process audio block
//     processBlock(n_frames);

//     // Get output
//     getOutput(output_buffers, n_channels, n_frames);
// }
void Synthesizer::processBlock(float** output_buffers, size_t n_channels, size_t n_frames) {
    // Clear buffers
    if(master_output_buffer) {
        master_output_buffer->zeroOut();
    }
    if(oversampled_buffer) {
        oversampled_buffer->zeroOut();
    }
    
    // Process at 2x sample rate (2x frames)
    size_t oversampled_frames = n_frames * m_context->oversampling;
    
    // Process all voices at 2x sample rate
    for(auto& voice : voices) {
        //if(!voice->isPlaying()) continue;
        voice->processVoice(oversampled_frames);
    }
    
    // Mix voices into oversampled buffer
    for(auto& voice : voices) {
        if(!voice->isPlaying()) continue;
        
        SignalBuffer* voice_buffer = voice->getMasterAudioBuffer();
        if(voice_buffer && oversampled_buffer) {
            for(size_t c = 0; c < oversampled_buffer->getNumChannels(); ++c) {
                float* oversample_channel = oversampled_buffer->getChannel(c);
                float* voice_channel = voice_buffer->getChannel(c % voice_buffer->getNumChannels());
                if(oversample_channel && voice_channel) {
                    for(size_t f = 0; f < oversampled_frames; ++f) {
                        oversample_channel[f] += voice_channel[f] * master_amplitude;
                    }
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
            std::memcpy(out_channel, master_channel, n_frames * sizeof(float));
        }
    }
}

void Synthesizer::prepare(size_t n_channels, size_t n_frames, float sample_rate){
    m_context->max_n_frames = n_frames * m_context->oversampling;
    m_context->sample_rate = sample_rate * static_cast<double>(m_context->oversampling);
    initializeOversampling(n_channels);
    for(auto& voice : voices){
        voice->setMasterAudioBufferInfo(n_channels);
        voice->resizeBuffers(m_context->max_n_frames);
        voice->setSampleRate(m_context->sample_rate);
    }
    setMasterOutputBufferInfoCallback(n_channels);
    if(master_output_buffer) {
        master_output_buffer->resize(n_channels, n_frames);
    } else {
        master_output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, n_frames, n_channels);
    }

    if(oversampled_buffer){
        delete oversampled_buffer;
    }
    oversampled_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, n_frames * m_context->oversampling, n_channels);
    //master_output_buffer->resize(n_channels, n_frames);

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


} // namespace OrangeSodium
