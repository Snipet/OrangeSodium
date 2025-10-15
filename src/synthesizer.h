//Synthesizer implementation
#pragma once
#include "context.h"
#include "voice.h"
#include <memory>
#include "program.h"

namespace OrangeSodium {
template <typename T>
class Synthesizer {
public:
    Synthesizer(Context* context, size_t n_voices, float sample_rate = 44100.0f, size_t n_frames = 512);
    ~Synthesizer();

    void loadScript(std::string script_path);
    void buildSynthFromProgram();

    void setSampleRate(T sample_rate) { m_context->sample_rate = sample_rate; }
    T getSampleRate() const { return static_cast<T>(m_context->sample_rate); }



private:
    std::vector<std::unique_ptr<Voice<T>>> voices;
    Context* m_context;
    Program<T>* program;
    SignalBuffer<T>* master_output_buffer;

    //Callback function for setting io information for master_output_buffer
    std::function<void(size_t)> setMasterOutputBufferInfoCallback;
};
}