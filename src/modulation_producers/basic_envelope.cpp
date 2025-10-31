#include "basic_envelope.h"

namespace OrangeSodium {

BasicEnvelope::BasicEnvelope(Context* context, ObjectID id)
    : ModulationProducer(context, id),
      attack_time(0.01f),
      decay_time(0.1f),
      sustain_level(0.4f),
      release_time(0.2f) {
    is_retriggered = false;
    output_buffer = nullptr;

    modulation_output_names.resize(0);
    modulation_output_names.push_back("output");
}

BasicEnvelope::BasicEnvelope(Context* context, ObjectID id, float attack, float decay, float sustain, float release)
    : ModulationProducer(context, id),
      attack_time(attack),
      decay_time(decay),
      sustain_level(sustain),
      release_time(release) {
    is_retriggered = false;
    output_buffer = nullptr;

    modulation_output_names.resize(0);
    modulation_output_names.push_back("output");
}

BasicEnvelope::~BasicEnvelope() {}

void BasicEnvelope::processBlock(SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_frames) {
    if (is_retriggered) {
        current_stage = EStage::kAttack;
        is_retriggered = false;
    }
    float* output_buffer = outputs->getChannel(0);
    for (size_t i = 0; i < n_frames; ++i) {
        switch (current_stage) {
            case EStage::kIdle:
                state = 0.0f;
                break;
            case EStage::kAttack:
                state += 1.0f / (attack_time * sample_rate);
                release_level = state;
                if (state >= 1.0f) {
                    state = 1.0f;
                    current_stage = EStage::kDecay;
                }
                break;
            case EStage::kDecay:
                state -= (1.0f - sustain_level) / (decay_time * sample_rate);
                release_level = state;
                if (state <= sustain_level) {
                    state = sustain_level;
                    current_stage = EStage::kSustain;
                    
                }
                break;
            case EStage::kSustain:
                // Hold sustain level
                state = sustain_level;
                release_level = state;
                break;
            case EStage::kRelease:
                state -= release_level / (release_time * sample_rate);
                if (state <= 0.0f) {
                    state = 0.0f;
                    current_stage = EStage::kIdle;
                }
                break;
        }
        output_buffer[i + frame_offset] = state;
    }
    frame_offset += n_frames;
}

void BasicEnvelope::onSampleRateChange(float new_sample_rate) {
    sample_rate = new_sample_rate / static_cast<float>(output_buffer->getChannelDivision(0));
}

    

} // namespace OrangeSodium