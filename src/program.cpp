#include "program.h"
#include <fstream>
#include "constants.h"
#include <iostream>
#include "console_utility.h"
#include <sstream>
#include "synthesizer.h"
#include "json/include/nlohmann/json.hpp"
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

using json = nlohmann::json;

namespace OrangeSodium {

static lua_State* getLuaState(void* L_void) {
    return static_cast<lua_State*>(L_void);
}

// Forward declarations for table-to-JSON conversion
void lua_value_to_json(lua_State* L, int index, json& j);
void lua_table_to_json(lua_State* L, int index, json& j);
void json_value_to_lua(lua_State* L, const json& j);
void json_to_lua_table(lua_State* L, const json& j);

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

// static int l_set_master_output_buffer_channels(lua_State* L) {
//     // Set the number of channels for the master output buffer
//     // Arguments: n_channels (int)
//     // Returns: none

//     if (lua_gettop(L) < 1 || !lua_isinteger(L, 1)) {
//         return 0;
//     }
//     size_t n_channels = static_cast<size_t>(lua_tointeger(L, 1));
//     if (n_channels < 1) {
//         n_channels = 1;
//     }

//     // Get the Program instance from registry
//     lua_pushstring(L, "__program_instance");
//     lua_gettable(L, LUA_REGISTRYINDEX);
//     void* program_ptr = lua_touserdata(L, -1);
//     lua_pop(L, 1);

//     if (!program_ptr) {
//         return 0;
//     }

//     Program* program = static_cast<Program*>(program_ptr);
//     auto& callback = program->getMasterOutputBufferCallback();
//     if (callback) {
//         callback(n_channels);
//     }

//     return 1;
// }

// static int l_set_master_output_buffer_default(lua_State* L){
//     // Set the master output buffer to a default stereo buffer
//     // Arguments: none
//     // Returns: none

//     // Get the Program instance from registry
//     lua_pushstring(L, "__program_instance");
//     lua_gettable(L, LUA_REGISTRYINDEX);
//     void* program_ptr = lua_touserdata(L, -1);
//     lua_pop(L, 1);

//     if (!program_ptr) {
//         return 0;
//     }

//     Program* program = static_cast<Program*>(program_ptr);
//     auto& callback = program->getMasterOutputBufferCallback();
//     if (callback) {
//         callback(2); // Default to stereo
//     }

//     return 1;
// }

// static int l_set_voice_output_buffer_channels(lua_State* L) {
//     // Set the number of channels for the voice's master audio buffer
//     // Arguments: n_channels (int)
//     // Returns: none

//     if (lua_gettop(L) < 1 || !lua_isinteger(L, 1)) {
//         return 0;
//     }
//     size_t n_channels = static_cast<size_t>(lua_tointeger(L, 1));
//     if (n_channels < 1) {
//         n_channels = 1;
//     }

//     // Get the template voice pointer from registry
//     lua_pushstring(L, "__template_voice");
//     lua_gettable(L, LUA_REGISTRYINDEX);
//     void* voice_ptr = lua_touserdata(L, -1);
//     lua_pop(L, 1);

//     if (!voice_ptr) {
//         return 0;
//     }

//     Voice* voice = static_cast<Voice*>(voice_ptr);
//     voice->setMasterAudioBufferInfo(n_channels);

//     return 1;
// }

// static int l_set_voice_output_buffer_default(lua_State* L){
//     // Set the voice's master audio buffer to a default stereo buffer
//     // Arguments: none
//     // Returns: none

//     // Get the template voice pointer from registry
//     lua_pushstring(L, "__template_voice");
//     lua_gettable(L, LUA_REGISTRYINDEX);
//     void* voice_ptr = lua_touserdata(L, -1);
//     lua_pop(L, 1);

//     if (!voice_ptr) {
//         return 0;
//     }

//     Voice* voice = static_cast<Voice*>(voice_ptr);
//     voice->setMasterAudioBufferInfo(2); // Default to stereo

//     return 1;
// }

// static int l_config_default_io(lua_State* L) {
//     // Configure default IO: stereo master output and voice output
//     // Arguments: none
//     // Returns: none

//     // Get the Program instance from registry
//     lua_pushstring(L, "__program_instance");
//     lua_gettable(L, LUA_REGISTRYINDEX);
//     void* program_ptr = lua_touserdata(L, -1);
//     lua_pop(L, 1);

//     if (!program_ptr) {
//         return 0;
//     }

//     Program* program = static_cast<Program*>(program_ptr);
//     auto& callback = program->getMasterOutputBufferCallback();
//     if (callback) {
//         callback(2); // Default to stereo
//     }

//     // Get the template voice pointer from registry
//     lua_pushstring(L, "__template_voice");
//     lua_gettable(L, LUA_REGISTRYINDEX);
//     void* voice_ptr = lua_touserdata(L, -1);
//     lua_pop(L, 1);

//     if (voice_ptr) {
//         Voice* voice = static_cast<Voice*>(voice_ptr);
//         voice->setMasterAudioBufferInfo(2); // Default to stereo
//     }

//     return 1;
// }

static int l_add_voice_audio_buffer_to_master(lua_State* L) {
    // Connect the voice's master audio buffer to a source audio buffer
    // Arguments: buffer_id (int), master_buffer_id (int)
    // Returns: none

    if (lua_gettop(L) < 2 || !lua_isinteger(L, 1) || !lua_isinteger(L, 2)) {
        return 0;
    }
    ObjectID buffer_id = static_cast<ObjectID>(lua_tointeger(L, 1));
    ObjectID master_buffer_id = static_cast<ObjectID>(lua_tointeger(L, 2));

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!voice_ptr) {
        return 0;
    }

    Voice* voice = static_cast<Voice*>(voice_ptr);
    ErrorCode error = voice->addAudioBufferToMaster(buffer_id, master_buffer_id);
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

static int l_add_effect_filter(lua_State* l){
    // Add a filter effect to the target effects chain
    // Arguments:
    //   Version 1 (JSON): effect_chain_id (int), json_params (string)
    //   Version 2 (Numeric): effect_chain_id (int), filter_type (string), cutoff (float), resonance (float)
    // Returns ObjectID of the filter effect or nil on failure

    if(lua_gettop(l) < 2 || !lua_isinteger(l, 1)){
        lua_pushnil(l);
        return 1;
    }

    EffectChainIndex effect_chain_id = static_cast<EffectChainIndex>(lua_tointeger(l, 1));
    ObjectID filter_id = static_cast<ObjectID>(-1);

    // Get program from registry
    lua_pushstring(l, "__program_instance");
    lua_gettable(l, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(l, -1);
    lua_pop(l, 1);
    if (!program_ptr) {
        lua_pushnil(l);
        return 1;
    }
    Program* program = static_cast<Program*>(program_ptr);

    // Get the effect chain from the program
    EffectChain* effect_chain = program->getEffectChainByIndex(effect_chain_id);
    if (!effect_chain) {
        lua_pushnil(l);
        return 1;
    }

    // Check if second argument is a string
    if (lua_isstring(l, 2)) {
        // Check if we have only 2 arguments (JSON mode)
        if (lua_gettop(l) == 2) {
            // JSON mode: second argument is JSON string
            std::string json_params = lua_tostring(l, 2);
            filter_id = effect_chain->addEffectFilterJSON(json_params);
        } else {
            // Numeric mode with 4 arguments: filter_type, cutoff, resonance
            if (lua_gettop(l) < 4 || !lua_isnumber(l, 3) || !lua_isnumber(l, 4)) {
                lua_pushnil(l);
                return 1;
            }
            std::string filter_type = lua_tostring(l, 2);
            float cutoff = static_cast<float>(lua_tonumber(l, 3));
            float resonance = static_cast<float>(lua_tonumber(l, 4));
            filter_id = effect_chain->addEffectFilter(filter_type, cutoff, resonance);
        }
    } else {
        lua_pushnil(l);
        return 1;
    }

    // Return the filter ID
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

static int l_add_voice_effect_chain(lua_State* L){
    // Add a voice effect chain to the template voice
    // Arguments: number of channels (int), input_buffer_id (int), output_buffer_id (int)
    // Returns: effect_chain_id (int) or nil on failure
    if (lua_gettop(L) < 3 ||
        !lua_isinteger(L, 1) ||
        !lua_isinteger(L, 2) ||
        !lua_isinteger(L, 3)) {
        lua_pushnil(L);
        return 1;
    }
    size_t n_channels = static_cast<size_t>(lua_tointeger(L, 1));
    ObjectID input_buffer_id = static_cast<ObjectID>(lua_tointeger(L, 2));
    ObjectID output_buffer_id = static_cast<ObjectID>(lua_tointeger(L, 3));

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
    ObjectID effect_chain_id = voice->addEffectChain(n_channels, input_buffer_id, output_buffer_id);
    
    // Return the effect chain ID
    lua_pushinteger(L, effect_chain_id);
    return 1;
}

static int l_add_audio_buffer(lua_State* L){
    // Add an audio buffer to the synthesizer (NOT THE VOICE)
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
    Synthesizer* synthesizer = static_cast<Synthesizer*>(program->getParentSynthesizer());
    ObjectID id = synthesizer->addAudioBuffer(n_channels);
    lua_pushinteger(L, id);
    return 1;
}

static int l_add_audio_buffer_to_master(lua_State* L) {
    // Add an audio buffer (in the synthesizer, NOT THE VOICE) to the master output
    // Arguments: buffer_id (int)
    // Returns: none

    if (lua_gettop(L) < 1 || !lua_isinteger(L, 1)) {
        return 0;
    }
    ObjectID buffer_id = static_cast<ObjectID>(lua_tointeger(L, 1));
    
    // Get the Program instance from registry
    lua_pushstring(L, "__program_instance");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!program_ptr) {
        return 0;
    }
    Program* program = static_cast<Program*>(program_ptr);
    Synthesizer* synthesizer = static_cast<Synthesizer*>(program->getParentSynthesizer());
    ErrorCode error = synthesizer->assignAudioBufferToOutput(buffer_id);
    handle_error(L, error);
    return 1;
}

static int l_set_portamento(lua_State* L) {
    // Sets the portamento time and whether the voice should always glide
    // Arguments: time (float), always_glide (bool)
    // Returns: none
    if (lua_gettop(L) < 2 || !lua_isnumber(L, 1) || !lua_isboolean(L, 2)) {
        return 0;
    }
    float time = static_cast<float>(lua_tonumber(L, 1));
    bool always_glide = lua_toboolean(L, 2);

    // Get the template voice pointer from registry
    lua_pushstring(L, "__template_voice");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* voice_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!voice_ptr) {
        return 0;
    }
    Voice* voice = static_cast<Voice*>(voice_ptr);
    voice->setPortamentoTime(time);
    voice->setAlwaysGlide(always_glide);
    return 1;
}

static int l_add_effect_chain(lua_State* L) {
    // Add an effect chain to the synthesizer (NOT THE VOICE)
    // Arguments: n_channels (int), input_buffer_id (int), output_buffer_id (int)
    // Returns: effect_chain_id (int) or nil on failure

    if (lua_gettop(L) < 3 || !lua_isinteger(L, 1) || !lua_isinteger(L, 2) || !lua_isinteger(L, 3)) {
        lua_pushnil(L);
        return 1;
    }

    size_t n_channels = static_cast<size_t>(lua_tointeger(L, 1));
    ObjectID input_buffer_id = static_cast<ObjectID>(lua_tointeger(L, 2));
    ObjectID output_buffer_id = static_cast<ObjectID>(lua_tointeger(L, 3));

    if (n_channels < 1) {
        n_channels = 1;
    }

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
    Synthesizer* synthesizer = static_cast<Synthesizer*>(program->getParentSynthesizer());
    EffectChainIndex id = synthesizer->addEffectChain(n_channels, input_buffer_id, output_buffer_id);
    lua_pushinteger(L, id);
    return 1;
}

//------------------------------------------------------- LUA JSON FUNCTIONS

// Convert a Lua table to a JSON object or array
void lua_table_to_json(lua_State* L, int index, json& j) {
    // Adjust index if negative (relative to top)
    if (index < 0) {
        index = lua_gettop(L) + index + 1;
    }

    // Check if the table is an array (all keys are consecutive integers starting from 1)
    bool is_array = true;
    lua_Integer expected_index = 1;
    lua_Integer max_index = 0;

    // First pass: check if it's an array
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        if (lua_isinteger(L, -2)) {
            lua_Integer key = lua_tointeger(L, -2);
            if (key > max_index) {
                max_index = key;
            }
        } else {
            is_array = false;
        }
        lua_pop(L, 1); // Pop value, keep key for next iteration
    }

    // Check for consecutive integers starting from 1
    if (is_array && max_index > 0) {
        lua_pushnil(L);
        lua_Integer count = 0;
        while (lua_next(L, index) != 0) {
            count++;
            lua_pop(L, 1);
        }
        if (count != max_index) {
            is_array = false;
        }
    }

    if (is_array && max_index > 0) {
        // Create JSON array
        j = json::array();
        for (lua_Integer i = 1; i <= max_index; i++) {
            lua_pushinteger(L, i);
            lua_gettable(L, index);
            json element;
            lua_value_to_json(L, -1, element);
            j.push_back(element);
            lua_pop(L, 1);
        }
    } else {
        // Create JSON object
        j = json::object();
        lua_pushnil(L);
        while (lua_next(L, index) != 0) {
            // Key is at -2, value is at -1
            std::string key;
            if (lua_isstring(L, -2)) {
                key = lua_tostring(L, -2);
            } else if (lua_isinteger(L, -2)) {
                key = std::to_string(lua_tointeger(L, -2));
            } else if (lua_isnumber(L, -2)) {
                key = std::to_string(lua_tonumber(L, -2));
            } else {
                lua_pop(L, 1);
                continue;
            }

            json value;
            lua_value_to_json(L, -1, value);
            j[key] = value;
            lua_pop(L, 1); // Pop value, keep key for next iteration
        }
    }
}


// Helper function to convert a Lua value to a JSON value
void lua_value_to_json(lua_State* L, int index, json& j) {
    int type = lua_type(L, index);

    switch (type) {
        case LUA_TNIL:
            j = nullptr;
            break;
        case LUA_TBOOLEAN:
            j = static_cast<bool>(lua_toboolean(L, index));
            break;
        case LUA_TNUMBER:
            if (lua_isinteger(L, index)) {
                j = lua_tointeger(L, index);
            } else {
                j = lua_tonumber(L, index);
            }
            break;
        case LUA_TSTRING:
            j = lua_tostring(L, index);
            break;
        case LUA_TTABLE:
            lua_table_to_json(L, index, j);
            break;
        default:
            j = nullptr;
            break;
    }
}

// Helper function to convert a JSON value to a Lua value
void json_value_to_lua(lua_State* L, const json& j) {
    if (j.is_null()) {
        lua_pushnil(L);
    }
    else if (j.is_boolean()) {
        lua_pushboolean(L, j.get<bool>());
    }
    else if (j.is_number_integer()) {
        lua_pushinteger(L, j.get<lua_Integer>());
    }
    else if (j.is_number_float()) {
        lua_pushnumber(L, j.get<lua_Number>());
    }
    else if (j.is_string()) {
        lua_pushstring(L, j.get<std::string>().c_str());
    }
    else if (j.is_array() || j.is_object()) {
        json_to_lua_table(L, j);
    }
    else {
        lua_pushnil(L);
    }
}

// Convert a JSON object or array to a Lua table
void json_to_lua_table(lua_State* L, const json& j) {
    lua_newtable(L);
    
    if (j.is_array()) {
        // Handle JSON array - use 1-based indexing for Lua
        int index = 1;
        for (const auto& element : j) {
            lua_pushinteger(L, index++);
            json_value_to_lua(L, element);
            lua_settable(L, -3);
        }
    }
    else if (j.is_object()) {
        // Handle JSON object
        for (auto it = j.begin(); it != j.end(); ++it) {
            lua_pushstring(L, it.key().c_str());
            json_value_to_lua(L, it.value());
            lua_settable(L, -3);
        }
    }
}

// Main Lua-bound function
// Takes a JSON string and returns a Lua table
int lua_json_to_table(lua_State* L) {
    // Check if we have exactly one argument
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Expected exactly one argument (JSON string)");
        return 2;
    }

    // Check if the argument is a string
    if (!lua_isstring(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument must be a string");
        return 2;
    }

    // Get the JSON string
    const char* json_str = lua_tostring(L, 1);

    try {
        // Parse the JSON string
        json j = json::parse(json_str);

        // Convert to Lua table
        json_to_lua_table(L, j);

        return 1; // Return the table
    }
    catch (const json::parse_error& e) {
        lua_pushnil(L);
        lua_pushstring(L, ("JSON parse error: " + std::string(e.what())).c_str());
        return 2;
    }
    catch (const std::exception& e) {
        lua_pushnil(L);
        lua_pushstring(L, ("Error: " + std::string(e.what())).c_str());
        return 2;
    }
}
// Main Lua-bound function
// Takes a Lua table and returns a JSON string
int lua_table_to_json(lua_State* L) {
    // Check if we have exactly one argument
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Expected exactly one argument (Lua table)");
        return 2;
    }

    // Check if the argument is a table
    if (!lua_istable(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument must be a table");
        return 2;
    }

    try {
        // Convert Lua table to JSON
        json j;
        lua_table_to_json(L, 1, j);

        // Convert JSON to string
        std::string json_str = j.dump();

        // Push the JSON string to Lua
        lua_pushstring(L, json_str.c_str());

        return 1; // Return the JSON string
    }
    catch (const std::exception& e) {
        lua_pushnil(L);
        lua_pushstring(L, ("Error: " + std::string(e.what())).c_str());
        return 2;
    }
}
//------------------------------------------------------------

static int l_add_effect_distortion(lua_State* L) {
    // Add a distortion effect to the target effects chain
    // Arguments:
    //  Version 1 (Lua Table) : effect_chain_id (int), lua_table_params (table)
    //  Version 2 (JSON String) : effect_chain_id (int), json_params (string)
    // Returns ObjectID of the distortion effect or nil on failure
    if(lua_gettop(L) < 2 || !lua_isinteger(L, 1)){
        lua_pushnil(L);
        return 1;
    }
    EffectChainIndex effect_chain_id = static_cast<EffectChainIndex>(lua_tointeger(L, 1));
    ObjectID distortion_id = static_cast<ObjectID>(-1);

    // Get program from registry
    lua_pushstring(L, "__program_instance");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* program_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!program_ptr) {
        lua_pushnil(L);
        return 1;
    }

    Program* program = static_cast<Program*>(program_ptr);
    // Get the effect chain from the program
    EffectChain* effect_chain = program->getEffectChainByIndex(effect_chain_id);
    if (!effect_chain) {
        lua_pushnil(L);
        return 1;
    }

    // Check if second argument is a table
    if (lua_istable(L, 2)) {
        // Lua Table mode
        json j;
        lua_table_to_json(L, 2, j);
        distortion_id = effect_chain->addEffectDistortionJSON(j.dump());
    } 
    else if (lua_isstring(L, 2)) {
        // JSON String mode
        std::string json_params = lua_tostring(L, 2);
        distortion_id = effect_chain->addEffectDistortionJSON(json_params);
    } 
    else {
        lua_pushnil(L);
        return 1;
    }
    // Return the distortion ID
    if(distortion_id == static_cast<ObjectID>(-1)){
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, distortion_id);
    }
}

//==========================================================================

Program::Program(Context* context, void* parent_synthesizer) : context(context), parent_synthesizer(parent_synthesizer), program_path(""), program_name("") {

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
    // lua_register(getLuaState(L), "config_master_output", l_set_master_output_buffer_channels);
    // lua_register(getLuaState(L), "config_master_output_default", l_set_master_output_buffer_default);
    // lua_register(getLuaState(L), "config_voice_output", l_set_voice_output_buffer_channels);
    // lua_register(getLuaState(L), "config_voice_output_default", l_set_voice_output_buffer_default);
    // lua_register(getLuaState(L), "config_default_io", l_config_default_io);
    lua_register(getLuaState(L), "get_connected_audio_buffer_for_oscillator", l_get_connected_audio_buffer_for_oscillator);
    lua_register(getLuaState(L), "assign_oscillator_audio_buffer", l_assign_oscillator_audio_buffer);
    lua_register(getLuaState(L), "add_voice_output", l_add_voice_audio_buffer_to_master);
    lua_register(getLuaState(L), "add_basic_envelope", l_add_basic_envelope);
    lua_register(getLuaState(L), "add_modulation", l_add_modulation);
    lua_register(getLuaState(L), "set_oscillator_frequency_offset", l_set_oscillator_frequency_offset);
    lua_register(getLuaState(L), "create_sawtooth_waveform", l_create_sawtooth_waveform);
    lua_register(getLuaState(L), "add_waveform_osc", l_add_waveform_osc);
    lua_register(getLuaState(L), "add_filter_effect", l_add_effect_filter);
    lua_register(getLuaState(L), "set_voice_rand_detune", l_set_voice_rand_detune);
    lua_register(getLuaState(L), "add_voice_effect_chain", l_add_voice_effect_chain);
    lua_register(getLuaState(L), "add_audio_buffer", l_add_audio_buffer);
    lua_register(getLuaState(L), "add_buffer_to_master", l_add_audio_buffer_to_master);
    lua_register(getLuaState(L), "set_portamento", l_set_portamento);
    lua_register(getLuaState(L), "add_effect_chain", l_add_effect_chain);
    lua_register(getLuaState(L), "json_to_table", lua_json_to_table);
    lua_register(getLuaState(L), "table_to_json", lua_table_to_json);

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

size_t Program::getNumVoicesDefined(){
    // Call Lua function get_num_voices() if it exists
    size_t num_voices = 0;
    lua_getglobal(getLuaState(L), "get_num_voices");
    if (lua_isfunction(getLuaState(L), -1)) {
        if (lua_pcall(getLuaState(L), 0, 1, 0) != LUA_OK) {
            *context->log_stream << "[Program] Lua error in get_num_voices: " << lua_tostring(getLuaState(L), -1) << std::endl;
            lua_pop(getLuaState(L), 1); // Pop error message
        } else {
            if (lua_isinteger(getLuaState(L), -1)) {
                num_voices = static_cast<size_t>(lua_tointeger(getLuaState(L), -1));
            }
            lua_pop(getLuaState(L), 1); // Pop return value
        }
    } else {
        lua_pop(getLuaState(L), 1); // Pop non-function
    }
    return num_voices;
}

Voice* Program::buildVoice() {
    Voice* voice = new Voice(context, parent_synthesizer);
    setTemplateVoice(voice);
    context->next_effect_chain_id = 0;

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

EffectChain* Program::getEffectChainByIndex(EffectChainIndex index) {
    if(index < 0){
        Synthesizer* synthesizer = static_cast<Synthesizer*>(parent_synthesizer);
        return synthesizer->getEffectChainByIndex(index);
    }else{
        return template_voice->getEffectChainByIndex(index);
    }
}

} // namespace OrangeSodium