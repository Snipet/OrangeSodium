#include "voice.h"
#include "oscillators/sine_osc.h"
#include "oscillators/waveform_osc.h"
#include "effects/effect_filter.h"
#include <cassert>
#include "synthesizer.h"
#include "console_utility.h"

namespace OrangeSodium{

static Synthesizer* getSynthesizerFromVoice(Voice* voice) {
    return static_cast<Synthesizer*>(voice->getParentSynthesizer());
}

Voice::Voice(Context* context, void* parent_synthesizer) : m_context(context), parent_synthesizer(parent_synthesizer)
{
    //voice_master_audio_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, context->max_n_frames, 2); // Default to stereo
    is_playing = false;
    is_releasing = false;
    voice_age = 0;
    voice_detune_semitones = 0.0f;
    portamento_time = 0.0f;
    should_reset_portamento = true;
    portamento_g = 0.f;
    should_always_glide = false;
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

    // Clean up effect chains
    for (auto* effect_chain : effect_chains) {
        delete effect_chain;
    }

    // Clean up buffers
    for (auto* buffer : audio_buffers) {
        delete buffer;
    }
}

ObjectID Voice::addSineOscillator(size_t n_channels, float amplitude) {
    ObjectID id = m_context->getNextObjectID();
    SineOscillator* osc = new SineOscillator(m_context, id, n_channels, amplitude);
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, 2); // Sine oscillator uses 2 mod channels (pitch, amplitude)
    //mod_buffer->setId(m_context->getNextObjectID());
    mod_buffer->setChannelDivision(0, 1); // Pitch channel at audio rate
    mod_buffer->setChannelDivision(1, 1); // Amplitude channel at audio rate
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
    mod_buffer->setChannelDivision(0, 1); // Pitch channel at audio rate
    mod_buffer->setChannelDivision(1, 1); // Amplitude channel at audio rate
    osc->setModBuffer(mod_buffer);
    oscillators.push_back(osc);
    oscillator_ids.push_back(id);
    return id;
}

ObjectID Voice::addBasicEnvelopeInternal(BasicEnvelope* env, ObjectID id) {
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, 4); // Basic envelope uses 4 mod channels (attack, decay, sustain, release)
    SignalBuffer* mod_out_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, 1); // Output buffer for envelope
    //mod_buffer->setId(m_context->getNextObjectID());
    //mod_out_buffer->setId(id);
    //mod_buffer->setChannelDivision(0, 1);
    mod_out_buffer->setChannelDivision(0, 1); // Envelope output at audio rate
    env->setOutputBuffer(mod_out_buffer);
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

    // Check effect chains
    for (auto& effect_chain : effect_chains) {
        if (effect_chain->getIndex() == id) {
            return EObjectType::kEffectChain;
        }

        // Check effects within effect chains
        if (effect_chain->objectIDIsEffect(id)) {
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
    
    // Resize effect chain buffers
    for (auto* effect_chain : effect_chains) {
        effect_chain->resizeBuffers(n_frames);
    }
}

ObjectID Voice::addAudioBuffer(size_t n_frames, size_t n_channels) {
    ObjectID id = m_context->getNextObjectID();
    SignalBuffer* buffer = new SignalBuffer(SignalBuffer::EType::kAudio, n_frames, n_channels);
    buffer->setId(id);
    audio_buffers.push_back(buffer);
    return id;
}

SignalBuffer* Voice::getAudioBufferByID(ObjectID id) {
    for (auto* buffer : audio_buffers) {
        if (buffer->getId() == id) {
            return buffer;
        }
    }
    return nullptr; // Not found
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

ErrorCode Voice::addAudioBufferToMaster(ObjectID buffer_id, ObjectID master_buffer_id) {
    // Find the buffer
    for (auto* buffer : audio_buffers) {
        if (buffer->getId() == buffer_id) {
            voice_master_audio_buffer_src_ptrs.push_back(buffer);

            // Now find the master buffer in the parent synthesizer
            Synthesizer* synth = getSynthesizerFromVoice(this);
            SignalBuffer* synth_audio_buffer = synth->getAudioBufferByID(master_buffer_id);
            if (synth_audio_buffer) {
                parent_audio_buffer_ptrs.push_back(synth_audio_buffer);
                //ConsoleUtility::logYellow(m_context->log_stream, "Connected voice buffer ID " + std::to_string(buffer_id) + " to master buffer ID " + std::to_string(master_buffer_id));
                
            } else {
                return ErrorCode::kAudioBufferNotFound;
            }
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
        EffectChain* parent_effect_chain = nullptr;
        EffectChainIndex parent_effect_chain_index = -1;

        for (auto* effect_chain : effect_chains){
            if (effect_chain->hasEffect(target_id)){
                parent_effect_chain = effect_chain;
                parent_effect_chain_index = effect_chain->getIndex();
                break;
            }
        }

        if(parent_effect_chain){
            target_eff = parent_effect_chain->getEffectByObjectID(target_id);
        }

        if (!target_eff) {
            return ErrorCode::kModulationDestinationNotFound;
        }

        int effect_index = parent_effect_chain->getEffectIndexByObjectID(target_id);
        

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
        mod->amount = amount;
        mod->modulation_source = source_ptr;
        mod->source_index = source_index;
        mod->dest_index = dest_index;
        mod->dest_type = dest_type;
        mod->effect_chain_index = parent_effect_chain_index;
        mod->effect_index = effect_index;
        modulations.push_back(mod);
    }
    return ErrorCode::kNoError;
}

EffectChainIndex Voice::addEffectChain(size_t n_channels, ObjectID input_buffer_id, ObjectID output_buffer_id) {
    EffectChainIndex chain_index = m_context->getNextEffectChainIndex();
    EffectChain* effect_chain = new EffectChain(m_context, n_channels, chain_index);
    SignalBuffer* input_buffer = getAudioBufferByID(input_buffer_id);
    SignalBuffer* output_buffer = getAudioBufferByID(output_buffer_id);
    effect_chain->setIO(input_buffer, output_buffer);
    effect_chains.push_back(effect_chain);
    effect_chain_indexes.push_back(chain_index);
    return chain_index;
}

void Voice::processVoice(size_t n_audio_frames) {
    // Zero out the audio buffers

    // Zero out the master audio buffer
    // if(voice_master_audio_buffer){
    //     voice_master_audio_buffer->zeroOut();
    // }

    if(should_retrigger){
        should_retrigger = false;
        for(auto* mod_prod : modulation_producers){
            mod_prod->onRetrigger();
        }
    }

    // Process modulation producers
    for(auto* mod_prod : modulation_producers){
        mod_prod->processBlock(mod_prod->getModBuffer(), mod_prod->getOutputBuffer(), n_audio_frames);
    }


    // We need to set modulation inputs for each oscillator
    // For now, just set pitch and amplitude based on current_midi_note
    const float pitch = static_cast<float>(current_midi_note);
    for(auto* osc : oscillators){
        SignalBuffer* mod_buffer = osc->getModBuffer();
        if(mod_buffer){
            if(true){ // Temporary until I implement no glide with frame offset
                float* pitch_channel = mod_buffer->getChannel(static_cast<size_t>(Oscillator::EModChannel::kPitch));
                if(pitch_channel){
                    for(size_t f = 0; f < n_audio_frames; ++f){
                        calculatePortamentoCoefficient();
                        current_note += (pitch - current_note) * portamento_g;
                        pitch_channel[f + frame_offset] = current_note + voice_detune_semitones;
                    }
                }
            }else {
                mod_buffer->setConstantValue(static_cast<size_t>(Oscillator::EModChannel::kPitch), pitch + voice_detune_semitones, frame_offset);
            }
            mod_buffer->setConstantValue(static_cast<size_t>(Oscillator::EModChannel::kAmplitude), 0.f, frame_offset);
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
                    dest_channel[dest_idx + frame_offset / dest_division] += mod->amount * source_channel[source_idx + frame_offset / source_division];
                }
            }
        }else if (mod->dest_type == EObjectType::kEffect){
            Effect* dest = effect_chains[mod->effect_chain_index]->getEffectByIndex(mod->effect_index);
            if(!dest) {
                continue;
            }
            
            SignalBuffer* source_buffer = source->getOutputBuffer();
            SignalBuffer* dest_buffer = dest->getModulationBuffer();
            if(!source_buffer || !dest_buffer) continue;
            float* source_channel = source_buffer->getChannel(mod->source_index);
            float* dest_channel = dest_buffer->getChannel(mod->dest_index);
            
            if(!source_channel || !dest_channel) continue;
            const size_t source_division = source_buffer->getChannelDivision(mod->source_index);
            const size_t dest_division = dest_buffer->getChannelDivision(mod->dest_index);
            // Apply modulation
            for(size_t i = 0; i < n_audio_frames / dest_division; ++i){
                size_t source_idx = i / source_division;
                if(source_idx < n_audio_frames){
                    dest_channel[i + frame_offset / dest_division] += mod->amount * source_channel[source_idx + frame_offset / source_division];
                }
            }
        }
    }

    // Process oscillators
    for(auto* osc : oscillators){
        osc->processBlock(nullptr, osc->getModBuffer(), osc->getOutputBuffer(), n_audio_frames); // For now, no inputs
    }

    // Process effect chains
    for(auto* effect_chain : effect_chains){
        effect_chain->processBlock(n_audio_frames);
    }

    // Copy data from source buffer to master audio buffer
    // if(voice_master_audio_buffer_src_ptrs.size() != 0 && voice_master_audio_buffer){
    //     size_t n_channels = voice_master_audio_buffer->getNumChannels();
    //     for(size_t c = 0; c < n_channels; ++c){
    //         float* dest_channel = voice_master_audio_buffer->getChannel(c);
    //         if(!dest_channel) continue;
    //         for(size_t buf_idx = 0; buf_idx < voice_master_audio_buffer_src_ptrs.size(); ++buf_idx){
    //             float* src_channel = voice_master_audio_buffer_src_ptrs[buf_idx]->getChannel(c);
    //             for(size_t f = 0; f < n_audio_frames; ++f){
    //                 if(src_channel){
    //                     dest_channel[f] += src_channel[f];
    //                 }
    //             }
    //         }
    //     }
    // }

    // Copy data from voice_master_audio_buffer_src_ptrs and add them to parent_audio_buffer_ptrs
    // Both vectors are the same size and correspond to each other
    for (size_t i = 0; i < voice_master_audio_buffer_src_ptrs.size(); ++i) {
        SignalBuffer* src_buffer = voice_master_audio_buffer_src_ptrs[i];
        SignalBuffer* dest_buffer = parent_audio_buffer_ptrs[i];
        if (!src_buffer || !dest_buffer) {
            continue;
        }

        size_t n_channels = std::min(src_buffer->getNumChannels(), dest_buffer->getNumChannels()); // Because I gave up on doing more than two channels, this probably isn't necessary
        for (size_t c = 0; c < n_channels; ++c) {
            float* src_channel = src_buffer->getChannel(c);
            float* dest_channel = dest_buffer->getChannel(c);
            if (!src_channel || !dest_channel) {
                continue;
            }

            for (size_t f = 0; f < n_audio_frames; ++f) {
                dest_channel[f + frame_offset] += src_channel[f + frame_offset];
            }
        }
    }

    // Release the voice if it's deactivated and there is no sound
    if(is_releasing){
        bool silent = true;
        for(auto* buffer : voice_master_audio_buffer_src_ptrs){
            for(size_t c = 0; c < buffer->getNumChannels(); ++c){
                float* channel = buffer->getChannel(c);
                for(size_t f = 0; f < n_audio_frames; ++f){
                    if(std::abs(channel[f + frame_offset]) > 0.0001f){
                        silent = false;
                        break;
                    }
                }
                if(!silent){
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
    frame_offset += n_audio_frames;
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
    for (auto* effect_chain : effect_chains) {
        effect_chain->setSampleRate(sample_rate);
    }
}


void Voice::connectVoiceEffects(){
    // for(size_t i = 0; i < effects.size(); ++i) {
    //     Effect* effect = effects[i];
    //     if(i == 0) {
    //         // First effect needs connected to voice_effects_input_buffer
    //         effect->setInputBuffer(voice_effects_input_buffer);
    //     }else {
    //         // Connect to previous effect's output
    //         effect->setInputBuffer(effects[i - 1]->getOutputBuffer());
    //     }
    // }

    // // Connect last effect to voice_effects_output_buffer
    // if(!effects.empty()) {
    //     effects.back()->setOutputBuffer(voice_effects_output_buffer);
    // }
    for(auto* effect_chain : effect_chains){
        effect_chain->connectEffects();
    }
}

EffectChain* Voice::getEffectChainByIndex(EffectChainIndex index) {
    for (auto* effect_chain : effect_chains) {
        if (effect_chain->getIndex() == index) {
            return effect_chain;
        }
    }
    return nullptr; // Not found
}

void Voice::beginBlock() {
    frame_offset = 0;

    for (auto* mod_prod : modulation_producers) {
        mod_prod->beginBlock();
    }

    for (auto* osc : oscillators) {
        osc->beginBlock();
    }

    for (auto* effect_chain : effect_chains) {
        effect_chain->beginBlock();
    }

    for(auto* buffer : audio_buffers){
        if(buffer->getType() == SignalBuffer::EType::kAudio){
            buffer->zeroOut();
        }
    }

    // Zero out modulation buffers for effects in effect chains
    for(auto* effect_chain : effect_chains){
        effect_chain->zeroOutModulationBuffers();
    }

}

}