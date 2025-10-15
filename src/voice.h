// Synthesizer voice
#pragma once
#include <vector>
#include "filter.h"
#include "oscillator.h"
#include "signal_buffer.h"
#include "context.h"

namespace OrangeSodium{

template <typename T>
class Voice {
public:
    enum class EObjectType{
        kOscillator = 0,
        kFilter,
        kEffect,
        kModulatorProducer,
        kUndefined
    };
    Voice(Context* context);
    ~Voice();
    
    ObjectID addSineOscillator(size_t n_channels); // Add a sine wave oscillator to the voice; returns its ObjectID
    EObjectType getObjectType(ObjectID id); // Get the type of object with the given ID; returns kOscillator, kFilter, etc. Returns kUndefined if not found (default)
    void setMasterAudioBufferInfo(size_t n_channels) {
        if (voice_master_audio_buffer) {
            delete voice_master_audio_buffer;
        }
        voice_master_audio_buffer = new SignalBuffer<T>(SignalBuffer<T>::EType::kAudio, m_context->n_frames, n_channels);
    }

private:
    /// @brief Collection of all voice oscillators
    std::vector<Oscillator<T>*> oscillators;
    std::vector<ObjectID> oscillator_ids;

    /// @brief Collection of all voice filters
    std::vector<Filter<T>*> filters;
    std::vector<ObjectID> filter_ids;

    /// @brief Collection of interconnecting signal buffers
    std::vector<SignalBuffer<T>*> audio_buffers;
    std::vector<SignalBuffer<T>*> mod_buffers;

    SignalBuffer<T>* voice_master_audio_buffer = nullptr; // Master audio output buffer for the voice
    

    Context* m_context;
};

}