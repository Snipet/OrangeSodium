#pragma once

#include "synthesizer.h"

namespace OrangeSodium{

Synthesizer* createSynthesizerFromScript(std::string script_path);
Synthesizer* createSynthesizerFromString(const std::string& script_data);

} // namespace OrangeSodium