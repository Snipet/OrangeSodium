#pragma once

namespace OrangeSodium{

typedef unsigned int ObjectID;
typedef unsigned int ResourceID;
typedef int EffectChainIndex; // Negative values indicate synthesizer-level chains
typedef unsigned int MasterBufferID;

enum EObjectType{
    kOscillator = 0,
    kEffect,
    kEffectChain,
    kModulatorProducer,
    kAudioBuffer,
    kUndefined
};

}