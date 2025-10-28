#include "program.h"
#include <fstream>
#include "constants.h"
#include <iostream>
#include "console_utility.h"
#include <sstream>
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

namespace OrangeSodium {

static lua_State* getLuaState(void* L_void) {
    return static_cast<lua_State*>(L_void);
}

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
    // Arguments:
    //   1. n_channels (int) - REQUIRED: number of output channels
    //   2. amplitude (float) - OPTIONAL: oscillator amplitude (0.0-1.0, default 1.0)
    //   3. buffer_id (int) - OPTIONAL: ObjectID for audio_buffer to route to
    // Returns: oscillator_id (int) or nil on failure

    // Argument 1: n_channels (REQUIRED)
    if (lua_gettop(L) < 1) {
        luaL_error(L, "add_sine_osc: missing required argument 'n_channels'");
        lua_pushnil(L);
        return 1;
    }

    if (!lua_isinteger(L, 1)) {
        luaL_error(L, "add_sine_osc: argument 1 'n_channels' must be an integer");
        lua_pushnil(L);
        return 1;
    }

    size_t n_channels = static_cast<size_t>(lua_tointeger(L, 1));
    if (n_channels < 1) {
        luaL_error(L, "add_sine_osc: 'n_channels' must be at least 1");
        lua_pushnil(L);
        return 1;
    }

    // Argument 2: amplitude (OPTIONAL, default 1.0)
    float amplitude = 1.0f;
    if (lua_gettop(L) >= 2) {
        if (!lua_isnumber(L, 2)) {
            luaL_error(L, "add_sine_osc: argument 2 'amplitude' must be a number");
            lua_pushnil(L);
            return 1;
        }
        amplitude = static_cast<float>(lua_tonumber(L, 2));
        if (amplitude < 0.0f || amplitude > 1.0f) {
            luaL_error(L, "add_sine_osc: 'amplitude' must be between 0.0 and 1.0");
            lua_pushnil(L);
            return 1;
        }
    }

    // Argument 3: buffer_id (OPTIONAL)
    ObjectID buffer_id = -1;
    bool has_buffer_arg = false;
    if (lua_gettop(L) >= 3) {
        if (!lua_isinteger(L, 3)) {
            luaL_error(L, "add_sine_osc: argument 3 'buffer_id' must be an integer");
            lua_pushnil(L);
            return 1;
        }
        buffer_id = static_cast<ObjectID>(lua_tointeger(L, 3));
        has_buffer_arg = true;
    }

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        luaL_error(L, "add_sine_osc: template voice not found");
        lua_pushnil(L);
        return 1;
    }

    Voice* voice = static_cast<Voice*>(voice_ptr);
    ObjectID id = voice->addSineOscillator(n_channels, amplitude);

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
    ObjectID id = voice->addAudioBuffer(context->max_n_frames, n_channels);
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
    EObjectType type = voice->getObjectType(id);

    switch(type){
        case EObjectType::kOscillator:
            lua_pushstring(L, "oscillator");
            break;
        case EObjectType::kEffect:
            lua_pushstring(L, "effect");
            break;
        case EObjectType::kModulatorProducer:
            lua_pushstring(L, "modulator_producer");
            break;
        case EObjectType::kAudioBuffer:
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

static int l_add_basic_envelope(lua_State* L){
    // Add a basic ADSR envelope modulation producer to the template voice
    // Arguments: none
    // Returns: modulation_producer_id (int) or nil on failure

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        lua_pushnil(L);
        return 1;
    }

    bool has_env_parameters = false;
    if (lua_gettop(L) >= 4) {
        has_env_parameters = true;
    }
    float attack, decay, sustain, release;
    if (has_env_parameters) {
        if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
            lua_pushnil(L);
            return 1;
        }
        attack = static_cast<float>(lua_tonumber(L, 1));
        decay = static_cast<float>(lua_tonumber(L, 2));
        sustain = static_cast<float>(lua_tonumber(L, 3));
        release = static_cast<float>(lua_tonumber(L, 4));
    }

    Voice* voice = static_cast<Voice*>(voice_ptr);
    ObjectID id;
    if (has_env_parameters) {
        id = voice->addBasicEnvelope(attack, decay, sustain, release);
    } else {
        id = voice->addBasicEnvelope();
    }

    lua_pushinteger(L, id);
    return 1;
}

static int l_add_modulation(lua_State* L){
    // Add a modulation from a source to a target parameter
    // Arguments:
    //   1. source_id (int)
    //   2. source_param (string)
    //   3. target_id (int)
    //   4. target_param (string)
    //   5. amount (float)
    //   6. is_centered (bool, optional, default false)
    // Returns: none

    if (lua_gettop(L) < 5 ||
        !lua_isinteger(L, 1) ||
        !lua_isstring(L, 2) ||
        !lua_isinteger(L, 3) ||
        !lua_isstring(L, 4) ||
        !lua_isnumber(L, 5)) {
        return 0;
    }

    ObjectID source_id = static_cast<ObjectID>(lua_tointeger(L, 1));
    std::string source_param = lua_tostring(L, 2);
    ObjectID target_id = static_cast<ObjectID>(lua_tointeger(L, 3));
    std::string target_param = lua_tostring(L, 4);
    float amount = static_cast<float>(lua_tonumber(L, 5));
    bool is_centered = false;
    if (lua_gettop(L) >= 6 && lua_isboolean(L, 6)) {
        is_centered = lua_toboolean(L, 6);
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
    ErrorCode error = voice->addModulation(source_id, source_param, target_id, target_param, amount, is_centered);
    handle_error(L, error);

    return 1;
}

static int l_set_oscillator_frequency_offset(lua_State* L) {
    // Set frequency offset (in MIDI note numbers) for the specified oscillator
    // Arguments: osc_id (int), midi_note_offset (float)
    // Returns: none

    if (lua_gettop(L) < 2 || !lua_isinteger(L, 1) || !lua_isnumber(L, 2)) {
        return 0;
    }
    ObjectID osc_id = static_cast<ObjectID>(lua_tointeger(L, 1));
    float midi_note_offset = static_cast<float>(lua_tonumber(L, 2));

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        return 0;
    }

    Voice* voice = static_cast<Voice*>(voice_ptr);
    ErrorCode error = voice->setOscillatorFrequencyOffset(osc_id, midi_note_offset);
    handle_error(L, error);

    return 1;
}

static int l_create_sawtooth_waveform(lua_State* L) {
    // Create a sawtooth waveform resource
    // Arguments: none
    // Returns: resource_id (int) or nil on failure

    // Get the Program instance from registry
    lua_pushstring(L, "__program_instance");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!program_ptr) {
        lua_pushnil(L);
        return 1;
    }

    Program* program = static_cast<Program*>(program_ptr);
    ResourceManager* resource_manager = program->getContext()->resource_manager;
    if (!resource_manager) {
        lua_pushnil(L);
        return 1;
    }

    ResourceID resource_id = resource_manager->createSawtoothWaveform();
    lua_pushinteger(L, resource_id);
    return 1;
}

static int l_add_waveform_osc(lua_State* L){
    // Add a waveform oscillator to the template voice
    // Arguments:
    //   1. n_channels (int) - REQUIRED: number of output channels
    //   2. waveform_id (int) - REQUIRED: ResourceID of the waveform to use
    //   3. amplitude (float) - OPTIONAL: oscillator amplitude (0.0-1.0, default 1.0)
    //   4. buffer_id (int) - OPTIONAL: ObjectID for audio_buffer to route to
    // Returns: oscillator_id (int) or nil on failure

    // Argument 1: n_channels (REQUIRED)
    if (lua_gettop(L) < 2) {
        luaL_error(L, "add_waveform_osc: missing required arguments");
        lua_pushnil(L);
        return 1;
    }

    if (!lua_isinteger(L, 1)) {
        luaL_error(L, "add_waveform_osc: argument 1 'n_channels' must be an integer");
        lua_pushnil(L);
        return 1;
    }

    size_t n_channels = static_cast<size_t>(lua_tointeger(L, 1));
    if (n_channels < 1) {
        luaL_error(L, "add_waveform_osc: 'n_channels' must be at least 1");
        lua_pushnil(L);
        return 1;
    }

    // Argument 2: waveform_id (REQUIRED)
    if (!lua_isinteger(L, 2)) {
        luaL_error(L, "add_waveform_osc: argument 2 'waveform_id' must be an integer");
        lua_pushnil(L);
        return 1;
    }
    ResourceID waveform_id = static_cast<ResourceID>(lua_tointeger(L, 2));

    // Argument 3: amplitude (OPTIONAL, default 1.0)
    float amplitude = 1.0f;
    if (lua_gettop(L) >= 3) {
        if (!lua_isnumber(L, 3)) {
            luaL_error(L, "add_waveform_osc: argument 3 'amplitude' must be a number");
            lua_pushnil(L);
            return 1;
        }
        amplitude = static_cast<float>(lua_tonumber(L, 3));
        if (amplitude < 0.0f || amplitude > 1.0f) {
            luaL_error(L, "add_waveform_osc: 'amplitude' must be between 0.0 and 1.0");
            lua_pushnil(L);
            return 1;
        }
    }

    // Argument 4: buffer_id (OPTIONAL)
    ObjectID buffer_id = static_cast<ObjectID>(luaL_optinteger(L, 4, -1));

    // Get the Program instance from registry
    lua_pushstring(L, "__program_instance");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!program_ptr) {
        lua_pushnil(L);
        return 1;
    }

    Program* program = static_cast<Program*>(program_ptr);
    Voice* voice = program->getTemplateVoice();
    if (!voice) {
        lua_pushnil(L);
        return 1;
    }

    ObjectID osc_id = voice->addWaveformOscillator(n_channels, waveform_id, amplitude);
    // If a buffer ID was provided, assign it to the oscillator
    if (buffer_id != static_cast<ObjectID>(-1)) {
        voice->assignOscillatorAudioBuffer(osc_id, buffer_id);
    }
    lua_pushinteger(L, osc_id);
    return 1;
}

static int l_configure_voice_effects_io(lua_State* L) {
    // Configure the IO for voice effects
    // Arguments: ObjectID of input buffer, ObjectID of output buffer
    if (lua_gettop(L) < 2 ||
        !lua_isinteger(L, 1) ||
        !lua_isinteger(L, 2)) {
        return 0;
    }
    ObjectID input_buffer_id = static_cast<ObjectID>(lua_tointeger(L, 1));
    ObjectID output_buffer_id = static_cast<ObjectID>(lua_tointeger(L, 2));

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!voice_ptr) {
        return 0;
    }
    Voice* voice = static_cast<Voice*>(voice_ptr);
    ErrorCode error = voice->configureVoiceEffectsIO(input_buffer_id, output_buffer_id);
    handle_error(L, error);
    return 1;
}

static int l_add_voice_effect_filter(lua_State* l){
    // Add a filter effect to the template voice's voice effects chain
    // Arguments: filter type (string), number of channels (int), cutoff (float), resonance (float)
    // Returns ObjectID of the filter effect or nil on failure
    if(lua_gettop(l) < 4 ||
       !lua_isstring(l, 1) ||
       !lua_isnumber(l, 2) ||
       !lua_isnumber(l, 3) ||
       !lua_isnumber(l, 4)){
        lua_pushnil(l);
        return 1;
    }
    std::string filter_type = lua_tostring(l, 1);
    int n_channels = static_cast<int>(lua_tointeger(l, 2));
    float cutoff = static_cast<float>(lua_tonumber(l, 3));
    float resonance = static_cast<float>(lua_tonumber(l, 4));

    // Get the template voice pointer from registry
    lua_pushstring(l, "__template_voice");
    lua_gettable(l, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(l, -1);
    lua_pop(l, 1);
    if (!voice_ptr) {
        lua_pushnil(l);
        return 1;
    }
    Voice* voice = static_cast<Voice*>(voice_ptr);
    ObjectID filter_id = voice->addEffectFilter(filter_type, n_channels, cutoff, resonance);
    if(filter_id == static_cast<ObjectID>(-1)){
        lua_pushnil(l);
    } else {
        lua_pushinteger(l, filter_id);
    }
    return 1;
}

static int l_set_voice_rand_detune(lua_State* L) {
    // Set the voice's random detune
    // Arguments: detune in semitones (float)
    // Returns: none
    if (lua_gettop(L) < 1 || !lua_isnumber(L, 1)) {
        return 0;
    }
    float detune_semitones = static_cast<float>(lua_tonumber(L, 1));
    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!voice_ptr) {
        return 0;
    }
    Voice* voice = static_cast<Voice*>(voice_ptr);
    voice->setRandomDetune(detune_semitones);
    return 1;
}

static int l_cpp_print(lua_State* L) {
    // Override Lua's print function to print to context's log stream
    int n = lua_gettop(L);  // Number of arguments

    // Get ostream from Program instance
    lua_pushstring(L, "__program_instance");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!program_ptr) {
        return 0;
    }
    Program* program = static_cast<Program*>(program_ptr);
    std::ostringstream ss;

    for (int i = 1; i <= n; ++i) {
        const char* s = lua_tostring(L, i);
        if (s)
            ss << s;
        else if (lua_isnil(L, i))
            ss << "nil";
        else if (lua_isboolean(L, i))
            ss << (lua_toboolean(L, i) ? "true" : "false");
        else
            ss << luaL_tolstring(L, i, nullptr), lua_pop(L, 1);
        if (i < n) ss << "\t";
    }
    *(program->getContext()->log_stream) << ss.str() << std::endl;
    return 0;
}

//==========================================================================

Program::Program(Context* context) : context(context), program_path(""), program_name("") {

    //Initialize Lua state
    L = luaL_newstate();
    if (!L) {
        throw std::runtime_error("Failed to create Lua state");
    }

    // Store this Program instance in Lua registry for access by Lua binding functions
    lua_pushstring(getLuaState(L), "__program_instance");
    lua_pushlightuserdata(getLuaState(L), this);
    lua_settable(getLuaState(L), LUA_REGISTRYINDEX);

    luaL_openlibs(getLuaState(L));

    // Register Lua functions
    lua_register(getLuaState(L), "get_osodium_version", l_get_osodium_version);
    lua_register(getLuaState(L), "add_sine_osc", l_add_sine_osc);
    lua_register(getLuaState(L), "add_voice_audio_buffer", l_add_voice_audio_buffer);
    lua_register(getLuaState(L), "get_object_type", l_get_object_type);
    lua_register(getLuaState(L), "config_master_output", l_set_master_output_buffer_channels);
    lua_register(getLuaState(L), "config_master_output_default", l_set_master_output_buffer_default);
    lua_register(getLuaState(L), "config_voice_output", l_set_voice_output_buffer_channels);
    lua_register(getLuaState(L), "config_voice_output_default", l_set_voice_output_buffer_default);
    lua_register(getLuaState(L), "config_default_io", l_config_default_io);
    lua_register(getLuaState(L), "get_connected_audio_buffer_for_oscillator", l_get_connected_audio_buffer_for_oscillator);
    lua_register(getLuaState(L), "assign_oscillator_audio_buffer", l_assign_oscillator_audio_buffer);
    lua_register(getLuaState(L), "set_voice_output", l_connect_voice_master_audio_buffer_to_source);
    lua_register(getLuaState(L), "add_basic_envelope", l_add_basic_envelope);
    lua_register(getLuaState(L), "add_modulation", l_add_modulation);
    lua_register(getLuaState(L), "set_oscillator_frequency_offset", l_set_oscillator_frequency_offset);
    lua_register(getLuaState(L), "create_sawtooth_waveform", l_create_sawtooth_waveform);
    lua_register(getLuaState(L), "add_waveform_osc", l_add_waveform_osc);
    lua_register(getLuaState(L), "configure_voice_effects_io", l_configure_voice_effects_io);
    lua_register(getLuaState(L), "add_voice_effect_filter", l_add_voice_effect_filter);
    lua_register(getLuaState(L), "set_voice_rand_detune", l_set_voice_rand_detune);

    // Override Lua print function
    lua_pushcfunction(getLuaState(L), l_cpp_print);
    lua_setglobal(getLuaState(L), "print");

}

Program::~Program() {
    if (L) {
        lua_close(getLuaState(L));
    }
}

void Program::setTemplateVoice(Voice* voice) {
    template_voice = voice;

    // Store the template voice in Lua registry for access by Lua binding functions
    if (L && voice) {
        lua_pushstring(getLuaState(L), "__template_voice");
        lua_pushlightuserdata(getLuaState(L), voice);
        lua_settable(getLuaState(L), LUA_REGISTRYINDEX);
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

bool Program::loadFromString(const std::string& program_data_str) {
    program_data = program_data_str;
    return true;
}

bool Program::execute() {
    if (!L) {
        return false;
    }
    if (luaL_dostring(getLuaState(L), program_data.c_str()) != LUA_OK) {
        *context->log_stream << "[Program] Lua error: " << lua_tostring(getLuaState(L), -1) << std::endl;
        lua_close(getLuaState(L));
        return false;
    }
    return true;
}

void Program::throwProgramError(ErrorCode code) {
    // Throw a program error with the given ErrorCode
    std::string message = OSGetErrorMessage(code);
    ConsoleUtility::logRed(context->log_stream, "[Program Error] " + message);
}

Voice* Program::buildVoice() {
    Voice* voice = new Voice(context);
    setTemplateVoice(voice);

    // Call Lua function build_voice() if it exists
    lua_getglobal(getLuaState(L), "build_voice");
    if (lua_isfunction(getLuaState(L), -1)) {
        if (lua_pcall(getLuaState(L), 0, 0, 0) != LUA_OK) {
            *context->log_stream << "[Program] Lua error in build_voice: " << lua_tostring(getLuaState(L), -1) << std::endl;
            lua_pop(getLuaState(L), 1); // Pop error message
        }
    } else {
        lua_pop(getLuaState(L), 1); // Pop non-function
    }
    voice->connectVoiceEffects();
    return voice;
}

} // namespace OrangeSodium