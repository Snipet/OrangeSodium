#pragma once
#include <string>

namespace OrangeSodium{

enum ErrorCode{
    kNoError = 0,
    kAudioBufferNotFound,
    kModulationSourceNotFound,
    kModulationDestinationNotFound,
    kModulationSourceParamNotFound,
    kModulationDestinationParamNotFound
};

static std::string OSGetErrorMessage(ErrorCode code){
    switch(code){
        case ErrorCode::kNoError:
            return "No error.";
        case ErrorCode::kAudioBufferNotFound:
            return "Audio buffer not found.";
        case ErrorCode::kModulationSourceNotFound:
            return "Modulation source not found.";
        case ErrorCode::kModulationDestinationNotFound:
            return "Modulation destination not found.";
        case ErrorCode::kModulationSourceParamNotFound:
            return "Modulation source parameter not found.";
        case ErrorCode::kModulationDestinationParamNotFound:
            return "Modulation destination parameter not found.";
        default:
            {
                std::string msg = "Unknown error code: " + std::to_string(static_cast<int>(code));
                return msg;
            }
    }
}
}