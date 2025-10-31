#include "effect_chain.h"
#include "effects/effect_filter.h"
#include "effects/effect_distortion.h"
#include "effects/effect_freqdiffuse.h"
#include "json/include/nlohmann/json.hpp"

using json = nlohmann::json;

namespace OrangeSodium {

EffectChain::EffectChain(Context* context, size_t n_channels, EffectChainIndex index)
    : m_context(context), n_channels(n_channels), index(index) {
}

ObjectID EffectChain::addEffectFilter(const std::string& filter_object_type, float frequency, float resonance) {
    ObjectID id = m_context->getNextObjectID();
    Filter::EFilterObjects filter_type = Filter::getFilterObjectTypeFromString(filter_object_type);
    FilterEffect* effect = new FilterEffect(m_context, id, n_channels, filter_type);
    effect->getFilter()->setCutoff(frequency);
    effect->getFilter()->setResonance(resonance);

    // We only need to create the output bufer and modulation buffer; the input buffer will be automatically connected
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, MAX_FILTER_MOD_PARAMETERS);
    SignalBuffer* output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);

    // Set modulation buffer divisions
    mod_buffer->setChannelDivision(0, Filter::getDefaultDivisions()); // Cutoff channel at audio rate
    mod_buffer->setChannelDivision(1, Filter::getDefaultDivisions()); // Resonance channel at audio rate
    mod_buffer->setConstantValue(0, 0.f); // Default no modulation
    mod_buffer->setConstantValue(1, 0.f); // Default no modulation
    effect->setModulationBuffer(mod_buffer);
    effect->setOutputBuffer(output_buffer);
    effects.push_back(effect);
    effect_ids.push_back(id);
    return id;
}

ObjectID EffectChain::addEffectFilterJSON(const std::string& json_data) {
    json j;
    try {
        j = json::parse(json_data);
    } catch (json::parse_error& e) {
        *(m_context->log_stream) << "Error parsing JSON data for EffectFilter: " << e.what() << std::endl;
        return -1; // Indicate error
    }

    std::string filter_object_type = j.value("filter_object_type", "ZDF");
    float frequency = j.value("frequency", 1000.0f);
    float resonance = j.value("resonance", 0.5f);

    return addEffectFilter(filter_object_type, frequency, resonance);
}

ObjectID EffectChain::addEffectDistortionJSON(const std::string& json_data) {
    json j;
    try {
        j = json::parse(json_data);
    } catch (json::parse_error& e) {
        *(m_context->log_stream) << "Error parsing JSON data for SimpleDistortion: " << e.what() << std::endl;
        return -1; // Indicate error
    }

    std::string distortion_type_str = j.value("type", "tanh");
    float drive = j.value("drive", 1.0f);
    float mix = j.value("mix", 1.0f);
    float output_gain = j.value("output_gain", 1.0f);

    ObjectID id = m_context->getNextObjectID();
    DistortionEffect* effect = new DistortionEffect(m_context, id, n_channels);
    effect->setDistortionType(DistortionEffect::getDistortionTypeFromString(distortion_type_str));
    effect->setDrive(drive);
    effect->setMix(mix);
    effect->setOutputGain(output_gain);

    // We only need to create the output bufer and modulation buffer; the input buffer will be automatically connected
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, DistortionEffect::getMaxModulationChannels());

    SignalBuffer* output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);

    effect->setModulationBuffer(mod_buffer);
    effect->setOutputBuffer(output_buffer);
    effects.push_back(effect);
    effect_ids.push_back(id);
    return id;
}

ObjectID EffectChain::addEffectFreqDiffuseJSON(const std::string& json_data) {
    json j;
    try {
        j = json::parse(json_data);
    } catch (json::parse_error& e) {
        *(m_context->log_stream) << "Error parsing JSON data for FreqDiffuseEffect: " << e.what() << std::endl;
        return -1; // Indicate error
    }

    //float diffusion_amount = j.value("diffusion_amount", 0.5f);

    ObjectID id = m_context->getNextObjectID();
    FreqDiffuseEffect* effect = new FreqDiffuseEffect(m_context, id, n_channels, 24*4);
    //effect->setDiffusionAmount(diffusion_amount);

    // We only need to create the output bufer; the input buffer will be automatically connected
    SignalBuffer* output_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->max_n_frames, FreqDiffuseEffect::getMaxModulationChannels());

    effect->setModulationBuffer(mod_buffer);
    effect->setOutputBuffer(output_buffer);
    effects.push_back(effect);
    effect_ids.push_back(id);
    return id;
}

Effect* EffectChain::getEffectByIndex(size_t index) {
    //TODO: See how much this impacts performance
    if (index < effects.size()) {
        return effects[index];
    }
    return nullptr;
}

void EffectChain::processBlock(size_t n_audio_frames) {
    for (auto* effect : effects) {
        SignalBuffer* audio_input = effect->getInputBuffer();
        SignalBuffer* mod_input = effect->getModulationBuffer();
        SignalBuffer* output = effect->getOutputBuffer();
        effect->processBlock(audio_input, mod_input, output, n_audio_frames);
    }

    if(effects.empty()) {
        // Copy input buffer to output buffer directly
        if(input_buffer && output_buffer) {
            for(size_t ch = 0; ch < n_channels; ++ch) {
                float* in_buf = input_buffer->getChannel(ch);
                float* out_buf = output_buffer->getChannel(ch);
                if(in_buf && out_buf) {
                    for(size_t i = 0; i < n_audio_frames; ++i) {
                        out_buf[i + frame_offset] = in_buf[i + frame_offset];
                    }
                }
            }
        }
    }

    frame_offset += n_audio_frames;
}

void EffectChain::setSampleRate(float sample_rate) {
    for (auto* effect : effects) {
        effect->setSampleRate(sample_rate);
    }
}

void EffectChain::connectEffects() {
    for(size_t i = 0; i < effects.size(); ++i) {
        Effect* effect = effects[i];
        if(i == 0) {
            // First effect needs connected to voice_effects_input_buffer
            effect->setInputBuffer(input_buffer);
        }else {
            // Connect to previous effect's output
            effect->setInputBuffer(effects[i - 1]->getOutputBuffer());
        }
    }

    // Connect last effect to voice_effects_output_buffer
    if(!effects.empty()) {
        effects.back()->setOutputBuffer(output_buffer);
    }
}

void EffectChain::resizeBuffers(size_t n_frames) {
    for (auto* effect : effects) {
        SignalBuffer* input_buffer = effect->getInputBuffer();
        SignalBuffer* output_buffer = effect->getOutputBuffer();
        SignalBuffer* mod_buffer = effect->getModulationBuffer();

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

Effect* EffectChain::getEffectByObjectID(ObjectID id) {
    for (size_t i = 0; i < effects.size(); ++i) {
        if (effect_ids[i] == id) {
            return effects[i];
        }
    }
    return nullptr;
}

int EffectChain::getEffectIndexByObjectID(ObjectID id) {
    for (size_t i = 0; i < effects.size(); ++i) {
        if (effect_ids[i] == id) {
            return i;
        }
    }
    return -1; // Not found
}

void EffectChain::zeroOutModulationBuffers() {
    for (auto* effect : effects) {
        SignalBuffer* mod_buffer = effect->getModulationBuffer();
        if (mod_buffer) {
            mod_buffer->zeroOut();
        }
    }
}

EffectChain::~EffectChain() {
    for (auto* effect : effects) {
        delete effect;
    }
}

void EffectChain::beginBlock() {
    frame_offset = 0;
    for (auto* effect : effects) {
        effect->beginBlock();
    }
}

}