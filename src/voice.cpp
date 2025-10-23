#include "voice.h"
#include "oscillators/sine_osc.h"
#include "modulation_producers/basic_envelope.h"

namespace OrangeSodium{

Voice::Voice(Context* context) : m_context(context)
{
    voice_master_audio_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, context->max_n_frames, 2); // Default to stereo
}

Voice::~Voice()
{
    // Clean up oscillators
    for (auto* osc : oscillators) {
        delete osc;
    }
    // Clean up filters
    for (auto* filter : filters) {
        delete filter;
    }
    // Clean up buffers
    for (auto* buffer : audio_buffers) {
        delete buffer;
    }
    for (auto* buffer : mod_buffers) {
        delete buffer;
    }
}

ObjectID Voice::addSineOscillator(size_t n_channels, float amplitude) {
    ObjectID id = m_context->getNextObjectID();
    SineOscillator* osc = new SineOscillator(m_context, id, n_channels, amplitude);
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, 2); // Sine oscillator uses 2 mod channels (pitch, amplitude)
    //mod_buffer->setId(m_context->getNextObjectID());
    mod_buffer->setChannelDivision(0, 1); // Pitch channel at full sample rate
    mod_buffer->setChannelDivision(1, 1); // Amplitude channel at full sample rate
    mod_buffers.push_back(mod_buffer);
    osc->setModBuffer(mod_buffer);
    oscillators.push_back(osc);
    oscillator_ids.push_back(id);
    return id;
}

ObjectID Voice::addBasicEnvelope() {
    ObjectID id = m_context->getNextObjectID();
    BasicEnvelope* env = new BasicEnvelope(m_context, id);
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, 4); // Basic envelope uses 4 mod channels (attack, decay, sustain, release)
    SignalBuffer* mod_out_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, 1); // Output buffer for envelope
    //mod_buffer->setId(m_context->getNextObjectID());
    //mod_out_buffer->setId(id);
    //mod_buffer->setChannelDivision(0, 1);
    mod_out_buffer->setChannelDivision(0, 1); // Envelope output at full sample rate
    env->setOutputBuffer(mod_out_buffer);
    mod_buffers.push_back(mod_buffer);
    env->setModBuffer(mod_buffer);
    modulation_producers.push_back(env);
    modulation_producer_ids.push_back(id);
    return id;
}

EObjectType Voice::getObjectType(ObjectID id) {
    // Check oscillators
    for (const auto& osc_id : oscillator_ids) {
        if (osc_id == id) {
            return EObjectType::kOscillator;
        }
    }
    // Check filters
    for (const auto& filter_id : filter_ids) {
        // Assuming Filter class has an 'id' member similar to Oscillator
        if (filter_id == id) {
            return EObjectType::kFilter;
        }
    }
    
    // Check audio buffers
    for (const auto& buffer : audio_buffers) {
        if (buffer->getId() == id) {
            return EObjectType::kAudioBuffer; // Assuming audio buffers are considered effects here
        }
    }

    return EObjectType::kUndefined; // Not found
}

void Voice::resizeBuffers(size_t n_frames) {
    // Resize audio buffers
    for (auto* buffer : audio_buffers) {
        if (buffer->getType() == SignalBuffer::EType::kAudio) {
            for (size_t ch = 0; ch < buffer->getNumChannels(); ++ch) {
                buffer->setChannel(ch, n_frames, 1, buffer->getBufferId(ch));
            }
        }
    }

    // Resize oscillator buffers
    for (auto* osc : oscillators) {
        osc->resizeBuffers(n_frames);
    }

    // Resize modulation producer buffers
    for (auto* mod_prod : modulation_producers) {
        mod_prod->resizeBuffers(n_frames);
    }
}

ObjectID Voice::addAudioBuffer(size_t n_frames, size_t n_channels) {
    ObjectID id = m_context->getNextObjectID();
    SignalBuffer* buffer = new SignalBuffer(SignalBuffer::EType::kAudio, n_frames, n_channels);
    buffer->setId(id);
    audio_buffers.push_back(buffer);
    return id;
}

void Voice::assignOscillatorAudioBuffer(ObjectID osc_id, ObjectID buffer_id) {
    // Find the oscillator
    for (size_t i = 0; i < oscillators.size(); ++i) {
        if (oscillator_ids[i] == osc_id) {
            // Find the buffer
            for (auto* buffer : audio_buffers) {
                if (buffer->getId() == buffer_id) {
                    oscillators[i]->setOutputBuffer(buffer);
                    return;
                }
            }
        }
    }
}

ObjectID Voice::getConnectedAudioBufferForOscillator(ObjectID osc_id) {
    // Find the oscillator
    for (size_t i = 0; i < oscillators.size(); ++i) {
        if (oscillator_ids[i] == osc_id) {
            SignalBuffer* output_buffer = oscillators[i]->getOutputBuffer();
            if (output_buffer) {
                return output_buffer->getId();
            } else {
                return -1; // No buffer assigned
            }
        }
    }
    return -1; // Oscillator not found
}

ErrorCode Voice::connectMasterAudioBufferToSource(ObjectID buffer_id) {
    // Find the buffer
    for (auto* buffer : audio_buffers) {
        if (buffer->getId() == buffer_id) {
            voice_master_audio_buffer_src_ptr = buffer;
            return ErrorCode::kNoError;
        }
    }

    return ErrorCode::kAudioBufferNotFound;
}

ErrorCode Voice::addModulation(ObjectID source_id, std::string source_param, ObjectID target_id, std::string target_param, float amount, bool is_centered) {
    // Find the modulation source
    ModulationProducer* source_ptr = nullptr;
    for (auto* mod_prod : modulation_producers) {
        if (mod_prod->getId() == source_id) {
            source_ptr = mod_prod;
            break;
        }
    }
    if (!source_ptr) {
        return ErrorCode::kModulationSourceNotFound;
    }

    //We now have the source pointer, but now we need to find the source output index
    std::vector<std::string>& mod_output_names = source_ptr->getModulationOutputNames();
    size_t source_index = -1;

    for (size_t i = 0; i < mod_output_names.size(); ++i) {
        if (mod_output_names[i] == source_param) {
            source_index = i;
            break;
        }
    }

    if(source_index == -1){
        return ErrorCode::kModulationSourceParamNotFound;
    }

    // Find the modulation destination type
    EObjectType dest_type = getObjectType(target_id);
    if (dest_type == EObjectType::kUndefined) {
        return ErrorCode::kModulationDestinationNotFound;
    }
    
    // FOR NOW, only support modulation to oscillators
    if (dest_type == EObjectType::kOscillator){
        // Find the oscillator with the target_id
        Oscillator* target_osc = nullptr;
        for (size_t i = 0; i < oscillators.size(); ++i) {
            if (oscillator_ids[i] == target_id) {
                target_osc = oscillators[i];
                break;
            }
        }

        if (!target_osc) {
            return ErrorCode::kModulationDestinationNotFound;
        }

        // We now have the target oscillator. Now we need to find the index of the target modulation channel
        size_t dest_index = 0;
        bool found_param = false;
        std::vector<std::string>& mod_source_names = target_osc->getModulationSourceNames();
        for(size_t i = 0; i < mod_source_names.size(); ++i){
            if(mod_source_names[i] == target_param){
                dest_index = i;
                found_param = true;
                break;
            }
        }

        if(!found_param){
            return ErrorCode::kModulationDestinationParamNotFound;
        }

        // We now have the source and destination pointers, and we have the destination index
        Modulation* mod = new Modulation();
        mod->modulation_source = source_ptr;
        mod->modulation_destination = target_osc;
        mod->amount = amount;
        mod->source_index = source_index;
        mod->dest_index = dest_index;
        mod->dest_type = dest_type;
        modulations.push_back(mod);
    }
    return ErrorCode::kNoError;
}

void Voice::processVoice(size_t n_audio_frames) {
    // Zero out the audio buffers
    for(auto* buffer : audio_buffers){
        if(buffer->getType() == SignalBuffer::EType::kAudio){
            buffer->zeroOut();
        }
    }

    if(should_retrigger){
        should_retrigger = false;
        for(auto* mod_prod : modulation_producers){
            mod_prod->onRetrigger();
        }
    }

    // Process modulation producers
    for(auto* mod_prod : modulation_producers){
        mod_prod->processBlock(mod_prod->getModBuffer(), mod_prod->getOutputBuffer());
    }


    // We need to set modulation inputs for each oscillator
    // For now, just set pitch and amplitude based on current_midi_note
    const float pitch_hz = getHzFromMIDINote(current_midi_note);
    for(auto* osc : oscillators){
        SignalBuffer* mod_buffer = osc->getModBuffer();
        if(mod_buffer){
            mod_buffer->setConstantValue(static_cast<size_t>(Oscillator::EModChannel::kPitch), pitch_hz);
            mod_buffer->setConstantValue(static_cast<size_t>(Oscillator::EModChannel::kAmplitude), 0.f);
        }
    }

    // Apply modulations
    for(auto* mod : modulations){
        ModulationProducer* source = static_cast<ModulationProducer*>(mod->modulation_source);
        if(!source) continue;
        if(mod->dest_type == EObjectType::kOscillator){
            Oscillator* dest = static_cast<Oscillator*>(mod->modulation_destination);
            if(!dest) continue;
            SignalBuffer* source_buffer = source->getOutputBuffer();
            SignalBuffer* dest_buffer = dest->getModBuffer();
            if(!source_buffer || !dest_buffer) continue;
            float* source_channel = source_buffer->getChannel(mod->source_index);
            float* dest_channel = dest_buffer->getChannel(mod->dest_index);
            if(!source_channel || !dest_channel) continue;
            const size_t source_division = source_buffer->getChannelDivision(mod->source_index);
            const size_t dest_division = dest_buffer->getChannelDivision(mod->dest_index);
            // Apply modulation
            for(size_t i = 0; i < n_audio_frames; ++i){
                size_t source_idx = i / source_division;
                size_t dest_idx = i / dest_division;
                if(source_idx < n_audio_frames && dest_idx < n_audio_frames){
                    dest_channel[dest_idx] += mod->amount * source_channel[source_idx];
                }
            }
        }
    }

    // Process oscillators
    for(auto* osc : oscillators){
        osc->processBlock(nullptr, osc->getModBuffer(), osc->getOutputBuffer(), n_audio_frames); // For now, no inputs
    }

    // Copy data from source buffer to master audio buffer
    if(voice_master_audio_buffer_src_ptr && voice_master_audio_buffer){
        size_t n_channels = voice_master_audio_buffer->getNumChannels();
        for(size_t c = 0; c < n_channels; ++c){
            float* src_channel = voice_master_audio_buffer_src_ptr->getChannel(c);
            float* dest_channel = voice_master_audio_buffer->getChannel(c);
            if(src_channel && dest_channel){
                std::memcpy(dest_channel, src_channel, n_audio_frames * sizeof(float));
            }
        }
    }

    // Release the voice if it's deactivated and there is no sound
    if(is_releasing){
        bool silent = true;
        for(size_t c = 0; c < voice_master_audio_buffer->getNumChannels(); ++c){
            float* channel = voice_master_audio_buffer->getChannel(c);
            for(size_t f = 0; f < n_audio_frames; ++f){
                if(std::abs(channel[f]) > 0.0001f){
                    silent = false;
                    break;
                }
            }
            if(!silent){
                break;
            }
        }
        if(silent){
            is_playing = false;
            is_releasing = false;
        }
    }
}

void Voice::deactivate() {
    is_releasing = true;
    should_retrigger = false;

    // Notify modulation producers of release
    for (auto* mod_prod : modulation_producers) {
        mod_prod->onRelease();
    }
}

void Voice::setSampleRate(float sample_rate) {
    for (auto* osc : oscillators) {
        osc->setSampleRate(sample_rate);
    }
}

}