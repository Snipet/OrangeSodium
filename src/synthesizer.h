//Synthesizer implementation
#pragma once
#include "context.h"
#include "voice.h"
#include <memory>
#include "program.h"

namespace OrangeSodium {
class Synthesizer {
public:
    Synthesizer(Context* context, size_t n_voices, float sample_rate = 44100.0f, size_t n_frames = 512);
    ~Synthesizer();

    void loadScript(std::string script_path);
    void buildSynthFromProgram();

    //void setSampleRate(float sample_rate) { m_context->sample_rate = sample_rate; }
    float getSampleRate() const { return static_cast<float>(m_context->sample_rate); }

    void processBlock(size_t n_audio_frames);
    void getOutput(float** output_buffers, size_t n_channels, size_t n_frames);
    void processBlock(float** output_buffers, size_t n_channels, size_t n_frames);
    void prepare(size_t n_channels, size_t n_frames, float sample_rate);
    void processMidiEvent(int midi_note, bool note_on);



private:
    std::vector<std::unique_ptr<Voice>> voices;
    Context* m_context;
    Program* program;
    SignalBuffer* master_output_buffer;

    //Callback function for setting io information for master_output_buffer
    std::function<void(size_t)> setMasterOutputBufferInfoCallback;
};
}