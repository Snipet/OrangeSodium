#pragma once

namespace OrangeSodium{

typedef unsigned int ObjectID;

enum EObjectType{
    kOscillator = 0,
    kFilter,
    kEffect,
    kModulatorProducer,
    kAudioBuffer,
    kUndefined
};

}