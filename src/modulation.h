#pragma once

// Defines structure for a modulation

#include "utilities.h"

namespace OrangeSodium{

template <typename T>
struct Modulation{
    bool centered = false;
    T amount = 0; // Modulation amount, in range [-1, 1]
    ObjectID mod_source_id = 0; // ObjectID of modulation source
    size_t param_index = 0; // Index of parameter being modulated
    size_t impl_index = 0; // Index of implementation variable being modulated
};

};