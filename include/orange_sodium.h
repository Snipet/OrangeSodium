#pragma once

#include "synthesizer.h"
#include "lua/lua.h"

namespace OrangeSodium{

template <typename T>
Synthesizer<T>* createSynthesizerFromScript(std::string script_path);

} // namespace OrangeSodium