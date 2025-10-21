#pragma once
#include <string>

namespace OrangeSodium{

enum ErrorCode{
    kNoError = 0,
    kAudioBufferNotFound,

};

static std::string OSGetErrorMessage(ErrorCode code){
    switch(code){
        case ErrorCode::kNoError:
            return "No error.";
        case ErrorCode::kAudioBufferNotFound:
            return "Audio buffer not found.";
        default:
            return "Unknown error.";
    }
}
}