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
    Program(Context* context, void* parent_synthesizer);
    ~Program();

    bool loadFromFile(const std::string& path);
    bool loadFromString(const std::string& program_data);
    std::string getProgramPath() const { return program_path; }
    std::string getProgramName() const { return program_name; }
    Context* getContext() const { return context; }
    void setTemplateVoice(Voice* voice);
    Voice* getTemplateVoice() const { return template_voice; }
    bool execute();

    void throwProgramError(ErrorCode code);

    Voice* buildVoice();

    size_t getNumVoicesDefined();

    std::ostream* getLogStream() const {
        return context->log_stream;
    }

    void* getParentSynthesizer() const {
        return parent_synthesizer;
    }

    EffectChain* getEffectChainByIndex(EffectChainIndex index);

private:
    std::string program_path;
    std::string program_name;
    Context* context;
    std::string program_data;
    void* L = nullptr; //Lua state
    Voice* template_voice = nullptr; //Template voice built by program
    void* parent_synthesizer = nullptr; //Pointer to parent synthesizer
};

}