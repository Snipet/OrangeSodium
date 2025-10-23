// Synthesizer voice
#pragma once
#include <vector>
#include "filter.h"
#include "oscillator.h"
#include "signal_buffer.h"
#include "context.h"
#include "error_handler.h"
#include <cmath>
#include "modulator_producer.h"
#include  "modulation.h"
#include "modulation_producers/basic_envelope.h"

namespace OrangeSodium{

class Voice {
public:
    Voice(Context* context);
    ~Voice();

    ObjectID addSineOscillator(size_t n_channels, float amplitude); // Add a sine wave oscillator to the voice; returns its ObjectID
    ObjectID addAudioBuffer(size_t n_frames, size_t n_channels); // Add an audio buffer to the voice; returns its ObjectID
    EObjectType getObjectType(ObjectID id); // Get the type of object with the given ID; returns kOscillator, kFilter, etc. Returns kUndefined if not found (default)
    void setMasterAudioBufferInfo(size_t n_channels) {
        if (voice_master_audio_buffer) {
            delete voice_master_audio_buffer;
        }
        voice_master_audio_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);
    }

    ObjectID addBasicEnvelope(); // Add a basic ADSR envelope as a modulation producer; returns its ObjectID
    ObjectID addBasicEnvelope(float attack_time, float decay_time, float sustain_level, float release_time); // Add a basic ADSR envelope as a modulation producer; returns its ObjectID

    /// @brief Resize all buffers in the voice to the specified number of frames
    void resizeBuffers(size_t n_frames);
    
    void assignOscillatorAudioBuffer(ObjectID osc_id, ObjectID buffer_id);

    ObjectID getConnectedAudioBufferForOscillator(ObjectID osc_id);

    ErrorCode connectMasterAudioBufferToSource(ObjectID buffer_id);

    void processVoice(size_t n_audio_frames);

    ErrorCode addModulation(ObjectID source_id, std::string source_param, ObjectID target_id, std::string target_param, float amount, bool is_centered = false);

    inline SignalBuffer* getMasterAudioBuffer() const {
        return voice_master_audio_buffer;
    }

    void setMidiNote(unsigned int midi_note) {
        current_midi_note = midi_note;
    }

    inline bool isPlaying() const {
        return is_playing;
    }

    void activate(int midi_note) {
        current_midi_note = midi_note;
        is_playing = true;
        is_releasing = false;
        should_retrigger = true;
        voice_age = 0;
    }

    void deactivate();

    unsigned int getCurrentMIDINote() const {
        return current_midi_note;
    }

    void incrementVoiceAge() {
        ++voice_age;
    }

    unsigned int getVoiceAge() const {
        return voice_age;
    }

    void setSampleRate(float sample_rate);

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

    /// @brief Collection of all voice modulation producers
    std::vector<ModulationProducer*> modulation_producers;
    std::vector<ObjectID> modulation_producer_ids;

    /// @brief Collection of modulations
    std::vector<Modulation*> modulations;

    SignalBuffer* voice_master_audio_buffer = nullptr; // Master audio output buffer for the voice. DATA MUST BE COPIED TO THIS BUFFER
    SignalBuffer* voice_master_audio_buffer_src_ptr = nullptr; // Pointer to the source buffer that feeds into the master audio buffer

    int current_midi_note = 69; // Default to A4 (440 Hz) (lol)

    bool is_playing = false; // Denotes if this voice is currently active
    bool is_releasing = false; // Denotes if this voice is in the release phase
    bool should_retrigger = false; // Denotes if the voice should retrigger envelopes, etc
    unsigned int voice_age = 0; // Age of the voice in number of MIDI activations

    Context* m_context;

    ObjectID addBasicEnvelopeInternal(BasicEnvelope* env, ObjectID id);
};

}