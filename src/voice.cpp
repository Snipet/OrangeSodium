#include "voice.h"
#include "oscillators/sine_osc.h"
#include "oscillators/waveform_osc.h"
#include "effects/effect_filter.h"

namespace OrangeSodium{

Voice::Voice(Context* context) : m_context(context)
{
    voice_master_audio_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, context->max_n_frames, 2); // Default to stereo
    is_playing = false;
    is_releasing = false;
    voice_age = 0;
    voice_detune_semitones = 0.0f;
}

Voice::~Voice()
{
    // Clean up oscillators
    for (auto* osc : oscillators) {
        delete osc;
    }

    // Clean up modulation producers
    for (auto* mod_prod : modulation_producers) {
        delete mod_prod;
    }

    // Clean up effects
    for (auto* eff : effects) {
        delete eff;
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

ObjectID Voice::addWaveformOscillator(size_t n_channels, ResourceID waveform_id, float amplitude) {
    ObjectID id = m_context->getNextObjectID();
    WaveformOscillator* osc = new WaveformOscillator(m_context, id, waveform_id, n_channels, amplitude);
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, 2); // Waveform oscillator uses 2 mod channels (pitch, amplitude)
    //mod_buffer->setId(m_context->getNextObjectID());
    mod_buffer->setChannelDivision(0, 1); // Pitch channel at full sample rate
    mod_buffer->setChannelDivision(1, 1); // Amplitude channel at full sample rate
    mod_buffers.push_back(mod_buffer);
    osc->setModBuffer(mod_buffer);
    oscillators.push_back(osc);
    oscillator_ids.push_back(id);
    return id;
}

ObjectID Voice::addEffectFilter(const std::string& filter_object_type, size_t n_channels, float frequency, float resonance) {
    ObjectID id = m_context->getNextObjectID();
    Filter::EFilterObjects filter_type = Filter::getFilterObjectTypeFromString(filter_object_type);
    FilterEffect* effect = new FilterEffect(m_context, id, n_channels, filter_type);
    effect->getFilter()->setCutoff(frequency);
    effect->getFilter()->setResonance(resonance);

    // We only need to create the output bufer and modulation buffer; the input buffer will be automatically connected
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, MAX_FILTER_MOD_PARAMETERS);
    SignalBuffer* output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);

    // Set modulation buffer divisions
    mod_buffer->setChannelDivision(0, 1); // Cutoff channel at full sample rate
    mod_buffer->setChannelDivision(1, 1); // Resonance channel at full sample rate
    mod_buffer->setConstantValue(0, 0.f); // Default no modulation
    mod_buffer->setConstantValue(1, 0.f); // Default no modulation
    effect->setModulationBuffer(mod_buffer);
    effect->setOutputBuffer(output_buffer);
    effects.push_back(effect);
    effect_ids.push_back(id);
    return id;
}

ObjectID Voice::addBasicEnvelopeInternal(BasicEnvelope* env, ObjectID id) {
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

ObjectID Voice::addBasicEnvelope() {
    ObjectID id = m_context->getNextObjectID();
    BasicEnvelope* env = new BasicEnvelope(m_context, id);
    return addBasicEnvelopeInternal(env, id);
}

ObjectID Voice::addBasicEnvelope(float attack_time, float decay_time, float sustain_level, float release_time) {
    ObjectID id = m_context->getNextObjectID();
    BasicEnvelope* env = new BasicEnvelope(m_context, id, attack_time, decay_time, sustain_level, release_time);
    return addBasicEnvelopeInternal(env, id);
}

EObjectType Voice::getObjectType(ObjectID id) {
    // Check oscillators
    for (const auto& osc_id : oscillator_ids) {
        if (osc_id == id) {
            return EObjectType::kOscillator;
        }
    }

    // Check effects
    for (const auto& eff : effects) {
        if (eff->getId() == id) {
            return EObjectType::kEffect;
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

    // Resize effect buffers
    for (auto* eff : effects) {
        SignalBuffer* input_buffer = eff->getInputBuffer();
        SignalBuffer* output_buffer = eff->getOutputBuffer();
        SignalBuffer* mod_buffer = eff->getModulationBuffer();

        if (input_buffer) {
            for (size_t ch = 0; ch < input_buffer->getNumChannels(); ++ch) {
                input_buffer->setChannel(ch, n_frames, 1, input_buffer->getBufferId(ch));
            }
        }

        if (output_buffer) {
            for (size_t ch = 0; ch < output_buffer->getNumChannels(); ++ch) {
                output_buffer->setChannel(ch, n_frames, 1, output_buffer->getBufferId(ch));
            }
        }

        if (mod_buffer) {
            for (size_t ch = 0; ch < mod_buffer->getNumChannels(); ++ch) {
                mod_buffer->setChannel(ch, n_frames, mod_buffer->getChannelDivision(ch), mod_buffer->getBufferId(ch));
            }
        }
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

ErrorCode Voice::configureVoiceEffectsIO(ObjectID input_buffer_id, ObjectID output_buffer_id) {
    // Find input buffer
    voice_effects_input_buffer = nullptr;
    voice_effects_output_buffer = nullptr;
    for (auto* buffer : audio_buffers) {
        if (buffer->getId() == input_buffer_id) {
            voice_effects_input_buffer = buffer;
            break;
        }
    }
    if (!voice_effects_input_buffer) {
        return ErrorCode::kAudioBufferNotFound;
    }

    // Find output buffer
    for (auto* buffer : audio_buffers) {
        if (buffer->getId() == output_buffer_id) {
            voice_effects_output_buffer = buffer;
            break;
        }
    }
    if (!voice_effects_output_buffer) {
        return ErrorCode::kAudioBufferNotFound;
    }

    return ErrorCode::kNoError;
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
    } else if (dest_type == EObjectType::kEffect) {
        // Find the effect with the target_id
        Effect* target_eff = nullptr;
        for (size_t i = 0; i < effects.size(); ++i) {
            if (effect_ids[i] == target_id) {
                target_eff = effects[i];
                break;
            }
        }

        if (!target_eff) {
            return ErrorCode::kModulationDestinationNotFound;
        }

        // We now have the target effect. Now we need to find the index of the target modulation channel
        size_t dest_index = 0;
        bool found_param = false;
        std::vector<std::string>& mod_source_names = target_eff->getModulationSourceNames();
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
        mod->modulation_destination = target_eff;
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
    const float pitch = static_cast<float>(current_midi_note);
    for(auto* osc : oscillators){
        SignalBuffer* mod_buffer = osc->getModBuffer();
        if(mod_buffer){
            mod_buffer->setConstantValue(static_cast<size_t>(Oscillator::EModChannel::kPitch), pitch + voice_detune_semitones);
            mod_buffer->setConstantValue(static_cast<size_t>(Oscillator::EModChannel::kAmplitude), 0.f);
        }
    }

    // Zero out modulation buffers for effects
    for(auto* eff : effects){
        SignalBuffer* mod_buffer = eff->getModulationBuffer();
        if(mod_buffer){
            mod_buffer->zeroOut();
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
        }else if (mod->dest_type == EObjectType::kEffect){
            Effect* dest = static_cast<Effect*>(mod->modulation_destination);
            if(!dest) continue;
            SignalBuffer* source_buffer = source->getOutputBuffer();
            SignalBuffer* dest_buffer = dest->getModulationBuffer();
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

    for(auto* eff : effects){
        eff->processBlock(eff->getInputBuffer(), eff->getModulationBuffer(), eff->getOutputBuffer(), n_audio_frames);
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

ErrorCode Voice::setOscillatorFrequencyOffset(ObjectID osc_id, float midi_note_offset) {
    // Find the oscillator
    for (size_t i = 0; i < oscillators.size(); ++i) {
        if (oscillator_ids[i] == osc_id) {
            // Set the frequency offset
            oscillators[i]->setFrequencyOffset(midi_note_offset);
            return ErrorCode::kNoError;
        }
    }
    return ErrorCode::kModulationDestinationNotFound;
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
    for (auto* mod_prod : modulation_producers) {
        mod_prod->onSampleRateChange(sample_rate);
    }
    for (auto* eff : effects) {
        eff->setSampleRate(sample_rate);
    }
}


void Voice::connectVoiceEffects(){
    for(size_t i = 0; i < effects.size(); ++i) {
        Effect* effect = effects[i];
        if(i == 0) {
            // First effect needs connected to voice_effects_input_buffer
            effect->setInputBuffer(voice_effects_input_buffer);
        }else {
            // Connect to previous effect's output
            effect->setInputBuffer(effects[i - 1]->getOutputBuffer());
        }
    }

    // Connect last effect to voice_effects_output_buffer
    if(!effects.empty()) {
        effects.back()->setOutputBuffer(voice_effects_output_buffer);
    }
}



}