//Class containing information and functions for loading and working with programs (Lua)
#pragma once
#include <string>
#include "context.h"
#include "voice.h"
#include "error_handler.h"
#include <functional>

namespace OrangeSodium{

class Program{
public:
    Program(Context* context);
    ~Program();

    bool loadFromFile(const std::string& path);
    bool loadFromString(const std::string& program_data);
    std::string getProgramPath() const { return program_path; }
    std::string getProgramName() const { return program_name; }
    Context* getContext() const { return context; }
    void setTemplateVoice(Voice* voice);
    Voice* getTemplateVoice() const { return template_voice; }
    bool execute();
    void setMasterOutputBufferInfoCallback(std::function<void(size_t)> callback) {
        master_output_buffer_callback = callback;
    }

    std::function<void(size_t)>& getMasterOutputBufferCallback() {
        return master_output_buffer_callback;
    }

    void throwProgramError(ErrorCode code);

    Voice* buildVoice();

    std::ostream* getLogStream() const {
        return context->log_stream;
    }

private:
    std::string program_path;
    std::string program_name;
    Context* context;
    std::string program_data;
    void* L = nullptr; //Lua state
    Voice* template_voice = nullptr; //Template voice built by program

    std::function<void(size_t)> master_output_buffer_callback; // Callback to set master output buffer info; void(size_t n_channels)
};

}