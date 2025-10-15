#include "program.h"
#include <fstream>
#include "constants.h"
#include <iostream>

namespace OrangeSodium {

// LUA BINDING FUNCTIONS ===================================================

static int l_get_osodium_version(lua_State* L) {
    lua_pushstring(L, OS_VERSION);
    return 1; // Number of return values
}

//==========================================================================

Program::Program(Context* context) : context(context), program_path(""), program_name("") {

    //Initialize Lua state
    L = luaL_newstate();
    if (!L) {
        throw std::runtime_error("Failed to create Lua state");
    }

    // Register Lua functions
    lua_register(L, "get_osodium_version", l_get_osodium_version);

}

Program::~Program() {
    if (L) {
        lua_close(L);
    }
}

bool Program::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    program_data = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return true;
}

bool Program::execute() {
    if (!L) {
        return false;
    }
    luaL_openlibs(L);
    if (luaL_dostring(L, program_data.c_str()) != LUA_OK) {
        lua_close(L);
        return false;
    }
    return true;
}

} // namespace OrangeSodium