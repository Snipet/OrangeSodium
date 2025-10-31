//Synthesizer implementation
#pragma once
#include "context.h"
#include "voice.h"
#include <memory>
#include "program.h"
#include "hiir/PolyphaseIir2Designer.h"
#include "hiir/Upsampler2xFpu.h"
#include "hiir/Downsampler2xFpu.h"

namespace OrangeSodium {
class Synthesizer {
public:
    Synthesizer(Context* context, float sample_rate = 44100.0f, size_t n_frames = 512);
    ~Synthesizer();

    void loadScript(std::string script_path);
    void loadScriptFromString(const std::string& script_data);
    void buildSynthFromProgram();

    //void setSampleRate(float sample_rate) { m_context->sample_rate = sample_rate; }
    float getSampleRate() const { return static_cast<float>(m_context->sample_rate) * static_cast<float>(m_context->oversampling); }

    void processBlock(float** output_buffers, size_t n_channels, size_t n_frames, size_t offset = 0);
    void prepare(size_t n_channels, size_t n_frames, float sample_rate);
    void processMidiEvent(int midi_note, bool note_on);
    void setLogStream(std::ostream* stream) {
        m_context->log_stream = stream;
    }

    SignalBuffer* getAudioBufferByID(ObjectID id);
    ObjectID addAudioBuffer(size_t n_channels);
    ErrorCode assignAudioBufferToOutput(ObjectID buffer_id);

    EffectChainIndex addEffectChain(size_t n_channels, ObjectID input_buffer_id, ObjectID output_buffer_id);
    void connectEffects();

    EffectChain* getEffectChainByIndex(EffectChainIndex index);

private:
    std::vector<std::unique_ptr<Voice>> voices;
    Context* m_context;
    Program* program;
    Voice* most_recent_voice;
    SignalBuffer* master_output_buffer;
    SignalBuffer* oversampled_buffer;
    std::vector<SignalBuffer*> audio_buffers; // THESE ARE OVERSAMPLED
    std::vector<SignalBuffer*> audio_output_buffer_ptrs; // These buffers point to the direct output

    // Master effect chains
    std::vector<EffectChain*> master_effect_chains;

    float master_amplitude;


    static constexpr int HIIR_COEFFS = 8; // or 12 for higher quality
    std::vector<hiir::Downsampler2xFpu<HIIR_COEFFS>> downsamplers;

    // Oversampling parameters
    double stopband_attenuation_db = 96.0;  // Stopband rejection in dB
    double transition_bandwidth = 0.01;     // Transition band (0.01 = 1% of Nyquist)

    void initializeOversampling(size_t n_channels);
    double* computeHIIRCoefficients();

    inline size_t getEffectChainIndex(EffectChainIndex chain_id) {
        return static_cast<size_t>(-chain_id - 1);
    }

};
}