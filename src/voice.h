// Synthesizer voice
#pragma once
#include <vector>
#include "filter.h"
#include "oscillator.h"
#include "signal_buffer.h"
#include "context.h"
#include "error_handler.h"
#include <cmath>

namespace OrangeSodium{

class Voice {
public:
    enum class EObjectType{
        kOscillator = 0,
        kFilter,
        kEffect,
        kModulatorProducer,
        kAudioBuffer,
        kUndefined
    };
    Voice(Context* context);
    ~Voice();

    ObjectID addSineOscillator(size_t n_channels); // Add a sine wave oscillator to the voice; returns its ObjectID
    ObjectID addAudioBuffer(size_t n_frames, size_t n_channels); // Add an audio buffer to the voice; returns its ObjectID
    EObjectType getObjectType(ObjectID id); // Get the type of object with the given ID; returns kOscillator, kFilter, etc. Returns kUndefined if not found (default)
    void setMasterAudioBufferInfo(size_t n_channels) {
        if (voice_master_audio_buffer) {
            delete voice_master_audio_buffer;
        }
        voice_master_audio_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->n_frames, n_channels);
    }

    /// @brief Resize all audio buffers in the voice to the specified number of frames
    void resizeAudioBuffers(size_t n_frames);
    
    void assignOscillatorAudioBuffer(ObjectID osc_id, ObjectID buffer_id);

    ObjectID getConnectedAudioBufferForOscillator(ObjectID osc_id);

    ErrorCode connectMasterAudioBufferToSource(ObjectID buffer_id);

    void processVoice(size_t n_audio_frames);

    inline SignalBuffer* getMasterAudioBuffer() const {
        return voice_master_audio_buffer;
    }

    void setMidiNote(unsigned int midi_note) {
        current_midi_note = midi_note;
    }

    inline bool isPlaying() const {
        return is_playing;
    }

    void activate(unsigned int midi_note) {
        current_midi_note = midi_note;
        is_playing = true;
    }

private:
    /// @brief Collection of all voice oscillators
    std::vector<Oscillator*> oscillators;
    std::vector<ObjectID> oscillator_ids;

    /// @brief Collection of all voice filters
    std::vector<Filter*> filters;
    std::vector<ObjectID> filter_ids;

    /// @brief Collection of interconnecting signal buffers
    std::vector<SignalBuffer*> audio_buffers;
    std::vector<SignalBuffer*> mod_buffers;


    SignalBuffer* voice_master_audio_buffer = nullptr; // Master audio output buffer for the voice. DATA MUST BE COPIED TO THIS BUFFER
    SignalBuffer* voice_master_audio_buffer_src_ptr = nullptr; // Pointer to the source buffer that feeds into the master audio buffer

    unsigned int current_midi_note = 69; // Default to A4 (440 Hz) (lol)

    bool is_playing = false;

    Context* m_context;

    float getHzFromMIDINote(int midi_note) {
        return 440.0f * std::pow(2.0f, (midi_note - 69) / 12.0f);
    }
};

}