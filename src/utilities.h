#pragma once

namespace OrangeSodium{

typedef unsigned int ObjectID;
typedef unsigned int ResourceID;

enum EObjectType{
    kOscillator = 0,
    kEffect,
    kModulatorProducer,
    kAudioBuffer,
    kUndefined
};

}