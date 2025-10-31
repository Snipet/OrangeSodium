// Abstraction of anything that produces a modulation signal
#pragma once
#include "context.h"
#include "signal_buffer.h"
#include "utilities.h"
#include <vector>
#include <string>

namespace OrangeSodium {

class ModulationProducer {
public:
    ModulationProducer(Context* context, ObjectID id);
    ~ModulationProducer();

    /// @brief Run the modulation
    /// @param mod_inputs External modulation inputs; mod_inputs[0] is a retrigger signal; everything else is implementation specific
    /// @param outputs Audio output of modulation producer
    virtual void processBlock(SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_frames) = 0;
    virtual void onSampleRateChange(float new_sample_rate) = 0;
    virtual void onRetrigger() = 0;
    virtual void onRelease() = 0;

    void setModBuffer(SignalBuffer* buffer) {
        modulation_buffer = buffer;
    }

    inline SignalBuffer* getModBuffer() const {
        return modulation_buffer;
    }

    void setOutputBuffer(SignalBuffer* buffer) {
        output_buffer = buffer;
    }
    inline SignalBuffer* getOutputBuffer() const {
        return output_buffer;
    }

    ObjectID getId() const {
        return id;
    }

    void resizeBuffers(size_t n_frames) {
        if (modulation_buffer) {
            // Get temporary channel divisions
            size_t* channel_divisions = new size_t[modulation_buffer->getNumChannels()];
            for (size_t c = 0; c < modulation_buffer->getNumChannels(); ++c) {
                channel_divisions[c] = modulation_buffer->getChannelDivision(c);
            }
            modulation_buffer->resize(modulation_buffer->getNumChannels(), n_frames);
            // Restore channel divisions
            for (size_t c = 0; c < modulation_buffer->getNumChannels(); ++c) {
                modulation_buffer->setChannelDivision(c, channel_divisions[c]);
            }
            delete[] channel_divisions;
        }

        if(output_buffer) {
            output_buffer->resize(output_buffer->getNumChannels(), n_frames);
        }
    }

    std::vector<std::string>& getModulationOutputNames() {
        return modulation_output_names;
    }

    void beginBlock() {
        frame_offset = 0;
    }
    size_t getFrameOffset() const {
        return frame_offset;
    }

protected:
    Context* m_context;
    SignalBuffer* modulation_buffer;
    SignalBuffer* output_buffer;
    float sample_rate;
    ObjectID id;
    EObjectType object_type;
    std::vector<std::string> modulation_output_names;

    size_t frame_offset; // To allow for per-sample MIDI events, we keep track of the current frame offset within the block being processed
};

}