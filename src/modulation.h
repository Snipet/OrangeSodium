#pragma once

// Defines structure for a modulation

#include "utilities.h"

namespace OrangeSodium{

struct Modulation{
    // bool centered = false;
    // float amount = 0; // Modulation amount, in range [-1, 1]
    // ObjectID mod_source_id = 0; // ObjectID of modulation source
    // size_t param_index = 0; // Index of parameter being modulated
    // size_t impl_index = 0; // Index of implementation variable being modulated


    void* modulation_source = nullptr; // Pointer to modulation producer sources
    size_t source_index = 0; // Index of modulation source channel
    void* modulation_destination = nullptr; // Pointer to modulation destination
    size_t dest_index = 0; // Index of destination modulation channel
    float amount = 0.0f; // Modulation amount
    EObjectType dest_type;

    bool dest_is_effect;

    // Used only if the destination is an effect inside an effect chain
    EffectChainIndex effect_chain_index;
    size_t effect_index;
};

};