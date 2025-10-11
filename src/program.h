//Class containing information and functions for loading and working with programs (Lua)
#pragma once
#include <string>
#include "context.h"

namespace OrangeSodium{

class Program{
public:
    Program(Context* context);
    ~Program();
    bool loadFromFile(const std::string& path);
    std::string getProgramPath() const { return program_path; }
    std::string getProgramName() const { return program_name; }
    Context* getContext() const { return context; }

private:
    std::string program_path;
    std::string program_name;
    Context* context;
    std::string program_data;
};

}