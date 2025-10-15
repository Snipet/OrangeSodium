//Class containing information and functions for loading and working with programs (Lua)
#pragma once
#include <string>
#include "context.h"
#include "voice.h"
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

namespace OrangeSodium{

template <typename T>
class Program{
public:
    // Singleton access
    static Program<T>* getInstance(Context* context = nullptr);
    static void destroyInstance();

    // Delete copy constructor and assignment operator
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    bool loadFromFile(const std::string& path);
    std::string getProgramPath() const { return program_path; }
    std::string getProgramName() const { return program_name; }
    Context* getContext() const { return context; }
    void setTemplateVoice(Voice<T>* voice);
    Voice<T>* getTemplateVoice() const { return template_voice; }
    bool execute();

private:
    Program(Context* context);
    ~Program();

    static Program<T>* instance;

    std::string program_path;
    std::string program_name;
    Context* context;
    std::string program_data;
    lua_State* L = nullptr; //Lua state
    Voice<T>* template_voice = nullptr; //Template voice built by program
};

// Static member declaration
template <typename T>
Program<T>* Program<T>::instance = nullptr;

}