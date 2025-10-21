#include "program.h"
#include <fstream>
#include "constants.h"
#include <iostream>
#include "console_utility.h"

namespace OrangeSodium {

// LUA BINDING FUNCTIONS ===================================================

static void handle_error(lua_State* L, ErrorCode code){
    if(code != ErrorCode::kNoError){
        // Get program instance to throw error
        lua_pushstring(L, "__program_instance");
        lua_gettable(L, LUA_REGISTRYINDEX);
        Program* program = static_cast<Program*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (program) {
            program->throwProgramError(code);
        }
    }
}

static int l_get_osodium_version(lua_State* L) {
    lua_pushstring(L, OS_VERSION);
    return 1; // Number of return values
}

static int l_add_sine_osc(lua_State* L){
    // Add a sine oscillator to the template voice
    // Arguments: n_channels (int) - number of output channels; ObectID for audio_buffer to route to (optional)
    // Returns: oscillator_id (int) or nil on failure

    // Get n_channels argument from Lua (default to 1 if not provided)
    size_t n_channels = 1;
    if (lua_gettop(L) >= 1 && lua_isinteger(L, 1)) {
        n_channels = static_cast<size_t>(lua_tointeger(L, 1));
        if (n_channels < 1) {
            n_channels = 1;
        }
    }

    // Check for optional second argument (audio buffer ObjectID)
    ObjectID buffer_id = -1;
    bool has_buffer_arg = false;
    if (lua_gettop(L) >= 2 && lua_isinteger(L, 2)) {
        buffer_id = static_cast<ObjectID>(lua_tointeger(L, 2));
        has_buffer_arg = true;
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

    Voice* voice = static_cast<Voice*>(voice_ptr);
    ObjectID id = voice->addSineOscillator(n_channels);

    // If a buffer ID was provided, assign it to the oscillator
    if (has_buffer_arg) {
        voice->assignOscillatorAudioBuffer(id, buffer_id);
    }

    lua_pushinteger(L, id);
    return 1;
}

static int l_get_connected_audio_buffer_for_oscillator(lua_State* L){
    // Get the audio buffer connected to the given oscillator
    // Arguments: osc_id (int)
    // Returns: buffer_id (int) or nil on failure

    if (lua_gettop(L) < 1 || !lua_isinteger(L, 1)) {
        lua_pushnil(L);
        return 1;
    }
    ObjectID osc_id = static_cast<ObjectID>(lua_tointeger(L, 1));

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        lua_pushnil(L);
        return 1;
    }

    Voice* voice = static_cast<Voice*>(voice_ptr);
    ObjectID buffer_id = voice->getConnectedAudioBufferForOscillator(osc_id);
    if (buffer_id == static_cast<ObjectID>(-1)) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, buffer_id);
    }
    return 1;
}

static int l_add_voice_audio_buffer(lua_State* L){
    // Add an audio buffer to the template voice
    // Arguments: n_channels (int)
    // n_frames is taken from context
    // Returns: buffer_id (int) or nil on failure
    if (lua_gettop(L) < 1 || !lua_isinteger(L, 1)) {
        lua_pushnil(L);
        return 1;
    }
    size_t n_channels = static_cast<size_t>(lua_tointeger(L, 1));
    if (n_channels < 1) {
        n_channels = 1;
    }

    // Get the Program instance from registry to access context
    lua_pushstring(L, "__program_instance");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!program_ptr) {
        lua_pushnil(L);
        return 1;
    }

    Program* program = static_cast<Program*>(program_ptr);
    Context* context = program->getContext();

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        lua_pushnil(L);
        return 1;
    }

    Voice* voice = static_cast<Voice*>(voice_ptr);
    ObjectID id = voice->addAudioBuffer(context->n_frames, n_channels);
    lua_pushinteger(L, id);
    return 1;
}

static int l_assign_oscillator_audio_buffer(lua_State* L) {
    // Assign an audio buffer to an oscillator's output
    // Arguments: osc_id (int), buffer_id (int)
    // Returns: none

    if (lua_gettop(L) < 2 || !lua_isinteger(L, 1) || !lua_isinteger(L, 2)) {
        return 0;
    }
    ObjectID osc_id = static_cast<ObjectID>(lua_tointeger(L, 1));
    ObjectID buffer_id = static_cast<ObjectID>(lua_tointeger(L, 2));

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        return 0;
    }

    Voice* voice = static_cast<Voice*>(voice_ptr);
    voice->assignOscillatorAudioBuffer(osc_id, buffer_id);

    return 1;
}

static int l_get_object_type(lua_State* L){
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

    Voice* voice = static_cast<Voice*>(voice_ptr);
    Voice::EObjectType type = voice->getObjectType(id);

    switch(type){
        case Voice::EObjectType::kOscillator:
            lua_pushstring(L, "oscillator");
            break;
        case Voice::EObjectType::kFilter:
            lua_pushstring(L, "filter");
            break;
        case Voice::EObjectType::kEffect:
            lua_pushstring(L, "effect");
            break;
        case Voice::EObjectType::kModulatorProducer:
            lua_pushstring(L, "modulator_producer");
            break;
        case Voice::EObjectType::kAudioBuffer:
            lua_pushstring(L, "audio_buffer");
            break;
        default:
            lua_pushnil(L);
            break;
    }
    return 1;
}

static int l_set_master_output_buffer_channels(lua_State* L) {
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

    Program* program = static_cast<Program*>(program_ptr);
    auto& callback = program->getMasterOutputBufferCallback();
    if (callback) {
        callback(n_channels);
    }

    return 1;
}


static int l_set_master_output_buffer_default(lua_State* L){
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

    Program* program = static_cast<Program*>(program_ptr);
    auto& callback = program->getMasterOutputBufferCallback();
    if (callback) {
        callback(2); // Default to stereo
    }

    return 1;
}

static int l_set_voice_output_buffer_channels(lua_State* L) {
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

    Voice* voice = static_cast<Voice*>(voice_ptr);
    voice->setMasterAudioBufferInfo(n_channels);

    return 1;
}

static int l_set_voice_output_buffer_default(lua_State* L){
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

    Voice* voice = static_cast<Voice*>(voice_ptr);
    voice->setMasterAudioBufferInfo(2); // Default to stereo

    return 1;
}

static int l_config_default_io(lua_State* L) {
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

    Program* program = static_cast<Program*>(program_ptr);
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
        Voice* voice = static_cast<Voice*>(voice_ptr);
        voice->setMasterAudioBufferInfo(2); // Default to stereo
    }

    return 1;
}

static int l_connect_voice_master_audio_buffer_to_source(lua_State* L) {
    // Connect the voice's master audio buffer to a source audio buffer
    // Arguments: buffer_id (int)
    // Returns: none

    if (lua_gettop(L) < 1 || !lua_isinteger(L, 1)) {
        return 0;
    }
    ObjectID buffer_id = static_cast<ObjectID>(lua_tointeger(L, 1));

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        return 0;
    }

    Voice* voice = static_cast<Voice*>(voice_ptr);
    ErrorCode error = voice->connectMasterAudioBufferToSource(buffer_id);
    handle_error(L, error);

    return 1;
}

//==========================================================================

// Singleton implementation
Program* Program::instance = nullptr;

Program* Program::getInstance(Context* context) {
    if (!instance && context) {
        instance = new Program(context);
    }
    return instance;
}

void Program::destroyInstance() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

Program::Program(Context* context) : context(context), program_path(""), program_name("") {

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
    lua_register(L, "add_sine_osc", l_add_sine_osc);
    lua_register(L, "add_voice_audio_buffer", l_add_voice_audio_buffer);
    lua_register(L, "get_object_type", l_get_object_type);
    lua_register(L, "config_master_output", l_set_master_output_buffer_channels);
    lua_register(L, "config_master_output_default", l_set_master_output_buffer_default);
    lua_register(L, "config_voice_output", l_set_voice_output_buffer_channels);
    lua_register(L, "config_voice_output_default", l_set_voice_output_buffer_default);
    lua_register(L, "config_default_io", l_config_default_io);
    lua_register(L, "get_connected_audio_buffer_for_oscillator", l_get_connected_audio_buffer_for_oscillator);
    lua_register(L, "assign_oscillator_audio_buffer", l_assign_oscillator_audio_buffer);
    lua_register(L, "set_voice_output", l_connect_voice_master_audio_buffer_to_source);

}

Program::~Program() {
    if (L) {
        lua_close(L);
    }
}

void Program::setTemplateVoice(Voice* voice) {
    template_voice = voice;

    // Store the template voice in Lua registry for access by Lua binding functions
    if (L && voice) {
        lua_pushstring(L, "__template_voice");
        lua_pushlightuserdata(L, voice);
        lua_settable(L, LUA_REGISTRYINDEX);
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
        std::cerr << "[Program] Lua error: " << lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return false;
    }
    return true;
}

void Program::throwProgramError(ErrorCode code) {
    // Throw a program error with the given ErrorCode
    std::string message = OSGetErrorMessage(code);
    ConsoleUtility::logRed("[Program Error] " + message);
}
} // namespace OrangeSodium