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

// Template wrapper for add_sine_osc
template <typename T>
static int l_add_sine_osc_impl(lua_State* L){
    // Add a sine oscillator to the template voice
    // Arguments: n_channels (int) - number of output channels
    // Returns: oscillator_id (int) or nil on failure

    // Get n_channels argument from Lua (default to 1 if not provided)
    size_t n_channels = 1;
    if (lua_gettop(L) >= 1 && lua_isinteger(L, 1)) {
        n_channels = static_cast<size_t>(lua_tointeger(L, 1));
        if (n_channels < 1) {
            n_channels = 1;
        }
    }

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        lua_pushnil(L);
        return 1;
    }

    Voice<T>* voice = static_cast<Voice<T>*>(voice_ptr);
    ObjectID id = voice->addSineOscillator(n_channels);
    lua_pushinteger(L, id);
    return 1;
}

// C function wrappers for each template type
static int l_add_sine_osc_float(lua_State* L) {
    return l_add_sine_osc_impl<float>(L);
}

static int l_add_sine_osc_double(lua_State* L) {
    return l_add_sine_osc_impl<double>(L);
}

static int l_get_object_type_impl(lua_State* L){
    // Get the type of object with the given ID
    // Arguments: object_id (int)
    // Returns: object_type (string) or nil on failure

    if (lua_gettop(L) < 1 || !lua_isinteger(L, 1)) {
        lua_pushnil(L);
        return 1;
    }
    ObjectID id = static_cast<ObjectID>(lua_tointeger(L, 1));

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        lua_pushnil(L);
        return 1;
    }

    Voice<float>* voice = static_cast<Voice<float>*>(voice_ptr);
    Voice<float>::EObjectType type = voice->getObjectType(id);

    switch(type){
        case Voice<float>::EObjectType::kOscillator:
            lua_pushstring(L, "oscillator");
            break;
        case Voice<float>::EObjectType::kFilter:
            lua_pushstring(L, "filter");
            break;
        case Voice<float>::EObjectType::kEffect:
            lua_pushstring(L, "effect");
            break;
        case Voice<float>::EObjectType::kModulatorProducer:
            lua_pushstring(L, "modulator_producer");
            break;
        default:
            lua_pushnil(L);
            break;
    }
    return 1;
}

static int l_get_object_type_float(lua_State* L) {
    return l_get_object_type_impl(L);
}
static int l_get_object_type_double(lua_State* L) {
    return l_get_object_type_impl(L);
}

template <typename T>
static int l_set_master_output_buffer_channels_impl(lua_State* L) {
    // Set the number of channels for the master output buffer
    // Arguments: n_channels (int)
    // Returns: none

    if (lua_gettop(L) < 1 || !lua_isinteger(L, 1)) {
        return 0;
    }
    size_t n_channels = static_cast<size_t>(lua_tointeger(L, 1));
    if (n_channels < 1) {
        n_channels = 1;
    }

    // Get the Program instance from registry
    lua_pushstring(L, "__program_instance");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!program_ptr) {
        return 0;
    }

    Program<T>* program = static_cast<Program<T>*>(program_ptr);
    auto& callback = program->getMasterOutputBufferCallback();
    if (callback) {
        callback(n_channels);
    }

    return 1;
}

static int l_set_master_output_buffer_channels_float(lua_State* L) {
    return l_set_master_output_buffer_channels_impl<float>(L);
}
static int l_set_master_output_buffer_channels_double(lua_State* L) {
    return l_set_master_output_buffer_channels_impl<double>(L);
}


template <typename T>
static int l_set_master_output_buffer_default_impl(lua_State* L){
    // Set the master output buffer to a default stereo buffer
    // Arguments: none
    // Returns: none

    // Get the Program instance from registry
    lua_pushstring(L, "__program_instance");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!program_ptr) {
        return 0;
    }

    Program<T>* program = static_cast<Program<T>*>(program_ptr);
    auto& callback = program->getMasterOutputBufferCallback();
    if (callback) {
        callback(2); // Default to stereo
    }

    return 1;
}
static int l_set_master_output_buffer_default_float(lua_State* L) {
    return l_set_master_output_buffer_default_impl<float>(L);
}
static int l_set_master_output_buffer_default_double(lua_State* L) {
    return l_set_master_output_buffer_default_impl<double>(L);
}

template <typename T>
static int l_set_voice_output_buffer_channels_impl(lua_State* L) {
    // Set the number of channels for the voice's master audio buffer
    // Arguments: n_channels (int)
    // Returns: none

    if (lua_gettop(L) < 1 || !lua_isinteger(L, 1)) {
        return 0;
    }
    size_t n_channels = static_cast<size_t>(lua_tointeger(L, 1));
    if (n_channels < 1) {
        n_channels = 1;
    }

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        return 0;
    }

    Voice<T>* voice = static_cast<Voice<T>*>(voice_ptr);
    voice->setMasterAudioBufferInfo(n_channels);

    return 1;
}
static int l_set_voice_output_buffer_channels_float(lua_State* L) {
    return l_set_voice_output_buffer_channels_impl<float>(L);
}
static int l_set_voice_output_buffer_channels_double(lua_State* L) {
    return l_set_voice_output_buffer_channels_impl<double>(L);
}

template <typename T>
static int l_set_voice_output_buffer_default_impl(lua_State* L){
    // Set the voice's master audio buffer to a default stereo buffer
    // Arguments: none
    // Returns: none

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        return 0;
    }

    Voice<T>* voice = static_cast<Voice<T>*>(voice_ptr);
    voice->setMasterAudioBufferInfo(2); // Default to stereo

    return 1;
}
static int l_set_voice_output_buffer_default_float(lua_State* L) {
    return l_set_voice_output_buffer_default_impl<float>(L);
}
static int l_set_voice_output_buffer_default_double(lua_State* L) {
    return l_set_voice_output_buffer_default_impl<double>(L);
}

template <typename T>
static int l_config_default_io_impl(lua_State* L) {
    // Configure default IO: stereo master output and voice output
    // Arguments: none
    // Returns: none

    // Get the Program instance from registry
    lua_pushstring(L, "__program_instance");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!program_ptr) {
        return 0;
    }

    Program<T>* program = static_cast<Program<T>*>(program_ptr);
    auto& callback = program->getMasterOutputBufferCallback();
    if (callback) {
        callback(2); // Default to stereo
    }

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (voice_ptr) {
        Voice<T>* voice = static_cast<Voice<T>*>(voice_ptr);
        voice->setMasterAudioBufferInfo(2); // Default to stereo
    }

    return 1;
}
static int l_config_default_io_float(lua_State* L) {
    return l_config_default_io_impl<float>(L);
}
static int l_config_default_io_double(lua_State* L) {
    return l_config_default_io_impl<double>(L);
}

//==========================================================================

// Singleton implementation
template <typename T>
Program<T>* Program<T>::getInstance(Context* context) {
    if (!instance && context) {
        instance = new Program<T>(context);
    }
    return instance;
}

template <typename T>
void Program<T>::destroyInstance() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

template <typename T>
Program<T>::Program(Context* context) : context(context), program_path(""), program_name("") {

    //Initialize Lua state
    L = luaL_newstate();
    if (!L) {
        throw std::runtime_error("Failed to create Lua state");
    }

    // Store this Program instance in Lua registry for access by Lua binding functions
    lua_pushstring(L, "__program_instance");
    lua_pushlightuserdata(L, this);
    lua_settable(L, LUA_REGISTRYINDEX);

    // Register Lua functions
    lua_register(L, "get_osodium_version", l_get_osodium_version);

    // Register template-specific functions
    if constexpr (std::is_same_v<T, float>) {
        lua_register(L, "add_sine_osc", l_add_sine_osc_float);
        lua_register(L, "get_object_type", l_get_object_type_float);
        lua_register(L, "config_master_output", l_set_master_output_buffer_channels_float);
        lua_register(L, "config_master_output_default", l_set_master_output_buffer_default_float);
        lua_register(L, "config_voice_output", l_set_voice_output_buffer_channels_float);
        lua_register(L, "config_voice_output_default", l_set_voice_output_buffer_default_float);
        lua_register(L, "config_default_io", l_config_default_io_float);
    } else if constexpr (std::is_same_v<T, double>) {
        lua_register(L, "add_sine_osc", l_add_sine_osc_double);
        lua_register(L, "get_object_type", l_get_object_type_double);
        lua_register(L, "config_master_output", l_set_master_output_buffer_channels_double);
        lua_register(L, "config_master_output_default", l_set_master_output_buffer_default_double);
        lua_register(L, "config_voice_output", l_set_voice_output_buffer_channels_double);
        lua_register(L, "config_voice_output_default", l_set_voice_output_buffer_default_double);
        lua_register(L, "config_default_io", l_config_default_io_double);
    }

}

template <typename T>
Program<T>::~Program() {
    if (L) {
        lua_close(L);
    }
}

template <typename T>
void Program<T>::setTemplateVoice(Voice<T>* voice) {
    template_voice = voice;

    // Store the template voice in Lua registry for access by Lua binding functions
    if (L && voice) {
        lua_pushstring(L, "__template_voice");
        lua_pushlightuserdata(L, voice);
        lua_settable(L, LUA_REGISTRYINDEX);
    }
}

template <typename T>
bool Program<T>::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    program_data = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return true;
}

template <typename T>
bool Program<T>::execute() {
    if (!L) {
        return false;
    }
    luaL_openlibs(L);
    if (luaL_dostring(L, program_data.c_str()) != LUA_OK) {
        std::cerr << "[Program] Lua error: " << lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return false;
    }
    return true;
}

// Explicit template instantiations
template class Program<float>;
template class Program<double>;

} // namespace OrangeSodium