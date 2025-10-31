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
#include "effect.h"
#include "effect_chain.h"

namespace OrangeSodium{

class Voice {
public:
    Voice(Context* context, void* parent_synthesizer);
    ~Voice();

    ObjectID addSineOscillator(size_t n_channels, float amplitude); // Add a sine wave oscillator to the voice; returns its ObjectID
    ErrorCode setOscillatorFrequencyOffset(ObjectID osc_id, float midi_note_offset); // Set frequency offset (in MIDI note numbers) for the specified oscillator
    ObjectID addWaveformOscillator(size_t n_channels, ResourceID waveform_id, float amplitude); // Add a waveform oscillator to the voice; returns its ObjectID
    ObjectID addAudioBuffer(size_t n_frames, size_t n_channels); // Add an audio buffer to the voice; returns its ObjectID
    SignalBuffer* getAudioBufferByID(ObjectID id); // Get pointer to audio buffer by its ObjectID; returns nullptr if not found
    EObjectType getObjectType(ObjectID id); // Get the type of object with the given ID; returns kOscillator, kFilter, etc. Returns kUndefined if not found (default)
    // void setMasterAudioBufferInfo(size_t n_channels) {
    //     if (voice_master_audio_buffer) {
    //         delete voice_master_audio_buffer;
    //     }
    //     voice_master_audio_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, m_context->max_n_frames, n_channels);
    // }

    EffectChainIndex addEffectChain(size_t n_channels, ObjectID input_buffer_id, ObjectID output_buffer_id);

    ObjectID addBasicEnvelope(); // Add a basic ADSR envelope as a modulation producer; returns its ObjectID
    ObjectID addBasicEnvelope(float attack_time, float decay_time, float sustain_level, float release_time); // Add a basic ADSR envelope as a modulation producer; returns its ObjectID

    /// @brief Resize all buffers in the voice to the specified number of frames
    void resizeBuffers(size_t n_frames);
    
    void assignOscillatorAudioBuffer(ObjectID osc_id, ObjectID buffer_id);

    ObjectID getConnectedAudioBufferForOscillator(ObjectID osc_id);

    // Connect a voice audio buffer to a master buffer
    ErrorCode addAudioBufferToMaster(ObjectID buffer_id, ObjectID master_buffer_id);

    void processVoice(size_t n_audio_frames);

    // Called by program when the voice is finished building.
    void connectVoiceEffects();

    ErrorCode addModulation(ObjectID source_id, std::string source_param, ObjectID target_id, std::string target_param, float amount, bool is_centered = false);

    // inline SignalBuffer* getMasterAudioBuffer() const {
    //     return voice_master_audio_buffer;
    // }

    void setMidiNote(unsigned int midi_note) {
        current_midi_note = midi_note;
    }

    inline bool isPlaying() const {
        return is_playing;
    }

    void setAlwaysGlide(bool always_glide) { should_always_glide = always_glide; }
    void setPortamentoTime(float time_in_seconds) { portamento_time = time_in_seconds;}

    void activate(int midi_note) {
        last_midi_note = current_midi_note;
        current_midi_note = midi_note;
        if(is_playing && !should_always_glide && !is_releasing){
            should_reset_portamento = false;
        } else {
            should_reset_portamento = true;
        }
        is_playing = true;
        is_releasing = false;
        should_retrigger = true;
        voice_age = 0;
        if(should_reset_portamento){
            should_reset_portamento = false;
            current_note = static_cast<float>(midi_note);
            last_midi_note = current_midi_note;
        }
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

    void setRandomDetune(float semitone_scale){
        float r = rand() / static_cast<float>(RAND_MAX); // [0, 1]
        voice_detune_semitones = (r * 2.0f - 1.0f) * semitone_scale; // [-semitone_scale, semitone_scale]
    }

    void setSampleRate(float sample_rate);

    std::vector<SignalBuffer*>& getVoiceOutputBuffers() {
        return voice_master_audio_buffer_src_ptrs;
    }

    void* getParentSynthesizer() {
        return parent_synthesizer;
    }

    EffectChain* getEffectChainByIndex(EffectChainIndex index);

    void beginBlock();

private:
    /// @brief Collection of all voice oscillators
    std::vector<Oscillator*> oscillators;
    std::vector<ObjectID> oscillator_ids;

    /// @brief Collection of all voice effects
    // std::vector<Effect*> effects;
    // std::vector<ObjectID> effect_ids;

    std::vector<EffectChain*> effect_chains;
    std::vector<EffectChainIndex> effect_chain_indexes;

    /// @brief Collection of interconnecting signal buffers
    std::vector<SignalBuffer*> audio_buffers;

    /// @brief Collection of all voice modulation producers
    std::vector<ModulationProducer*> modulation_producers;
    std::vector<ObjectID> modulation_producer_ids;

    /// @brief Collection of modulations
    std::vector<Modulation*> modulations;

    //SignalBuffer* voice_master_audio_buffer = nullptr; // Master audio output buffer for the voice. DATA MUST BE COPIED TO THIS BUFFER
    std::vector<SignalBuffer*> voice_master_audio_buffer_src_ptrs; // Pointers to source buffers that are used as outputs for the voice
    std::vector<SignalBuffer*> parent_audio_buffer_ptrs; // Pointers to parent synthesizer master audio buffers that this voice outputs to

    int current_midi_note = 69; // Default to A4 (440 Hz) (lol)
    int last_midi_note = 69;
    float current_note; // In MIDI note; used for glide calculations

    bool is_playing = false; // Denotes if this voice is currently active
    bool is_releasing = false; // Denotes if this voice is in the release phase
    bool should_retrigger = false; // Denotes if the voice should retrigger envelopes, etc
    unsigned int voice_age = 0; // Age of the voice in number of MIDI activations

    float portamento_time = 0.f; // Time to glide between notes, in seconds
    float portamento_g; // Portamento filter coefficient

    float voice_detune_semitones = 0.0f; // Global detune for the voice, in semitones

    Context* m_context;

    void* parent_synthesizer = nullptr;

    bool should_reset_portamento = true;
    bool should_always_glide = false;

    size_t frame_offset; // To allow for per-sample MIDI events, we keep track of the current frame offset within the block being processed

    ObjectID addBasicEnvelopeInternal(BasicEnvelope* env, ObjectID id);
    void calculatePortamentoCoefficient(){
        if(portamento_time <= 0.f) {
            portamento_g = 1.f;
        } else {
            float time_constant = portamento_time * m_context->sample_rate;
            portamento_g = 1.f - std::exp(-1.f / time_constant);
        }

        portamento_g = std::clamp(portamento_g, 0.f, 1.f);
    }
};

}