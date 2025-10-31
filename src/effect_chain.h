#pragma once

#include "context.h"
#include "effect.h"


namespace OrangeSodium {

class EffectChain {
public:
    EffectChain(Context* context, size_t n_channels, EffectChainIndex id);
    ~EffectChain();
    ObjectID addEffectFilter(const std::string& filter_object_type, float frequency, float resonance);
    ObjectID addEffectFilterJSON(const std::string& json_data);
    ObjectID addEffectDistortionJSON(const std::string& json_data);
    Effect* getEffectByIndex(size_t index);
    size_t getNumEffects() const { return effects.size(); }
    void setIO(SignalBuffer* input, SignalBuffer* output) {
        input_buffer = input;
        output_buffer = output;
    }
    EffectChainIndex getIndex() const { return index; }
    void setSampleRate(float sample_rate);
    void connectEffects();

    void processBlock(size_t n_audio_frames);

    bool hasEffect(ObjectID id) const {
        for (const auto& eff_id : effect_ids) {
            if (eff_id == id) {
                return true;
            }
        }
        return false;
    }

    bool objectIDIsEffect(ObjectID id) const {
        return hasEffect(id);
    }

    bool hasEffects() const {
        return !effects.empty();
    }

    Effect* getEffectByObjectID(ObjectID id);

    int getEffectIndexByObjectID(ObjectID id);

    void resizeBuffers(size_t n_frames);

    void zeroOutModulationBuffers();

    void beginBlock();
    size_t getFrameOffset() const {
        return frame_offset;
    }

private:
    Context* m_context;
    size_t n_channels;
    EffectChainIndex index;
    std::vector<Effect*> effects;
    std::vector<ObjectID> effect_ids;
    SignalBuffer* input_buffer = nullptr;
    SignalBuffer* output_buffer = nullptr;

    size_t frame_offset; // To allow for per-sample MIDI events, we keep track of the current frame offset within the block being processed
};

}