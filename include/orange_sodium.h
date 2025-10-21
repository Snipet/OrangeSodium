#pragma once

#include "synthesizer.h"
#include "lua/lua.h"

namespace OrangeSodium{

Synthesizer* createSynthesizerFromScript(std::string script_path);

} // namespace OrangeSodium