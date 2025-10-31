#pragma once
#include "../effect.h"
#include "../filter.h"

/// Effect that applies a filter to the audio input

namespace OrangeSodium {

class FilterEffect : public Effect {
public:
    FilterEffect(Context* context, ObjectID id, size_t n_channels, Filter::EFilterObjects filter_type);
    ~FilterEffect();
    void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) override;
    void onSampleRateChange(float new_sample_rate) override;

    void setInputBuffer(SignalBuffer* buffer) override {
        this->input_buffer = buffer;
    }

    void setOutputBuffer(SignalBuffer* buffer) override {
        this->output_buffer = buffer;
    }

    void setModulationBuffer(SignalBuffer* buffer) override {
        this->mod_buffer = buffer;
    }

    SignalBuffer* getInputBuffer() const override {
        return this->input_buffer;
    }

    SignalBuffer* getOutputBuffer() const override {
        return this->output_buffer;
    }
    SignalBuffer* getModulationBuffer() const override {
        return this->mod_buffer;
    }

    Filter* getFilter() const {
        return filter;
    }

    void beginBlock() override {
        frame_offset = 0;
        if (filter) {
            filter->beginBlock();
        }
    }

private:
    Filter* filter;
    Filter::EFilterObjects filter_object_type;
};
}