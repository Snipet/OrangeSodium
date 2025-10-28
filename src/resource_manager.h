#pragma once

/*
* Resource manager for waveforms, wavetables, samples, etc.
*/

#include <vector>
#include "utilities.h"

namespace OrangeSodium{

class Resource{
public:
    enum class EType{
        kWaveform = 0,
        kWavetable,
        kSample
    };
    Resource(EType type);
    virtual ~Resource();
    ResourceID getId() const{ return id; }
    void setId(int newId){ id = newId; }
    Resource::EType getType() const{ return type_; }

protected:
    EType type_;
    ResourceID id;
};

class WaveformResource : public Resource {
public:
    WaveformResource(Resource::EType type, size_t length);
    ~WaveformResource() override;
    void createSawtooth();
    float* getData() const { return data; }
private:
    float* data;
    size_t length;
};

class ResourceManager{
public:
    ResourceManager();
    ~ResourceManager();

    ResourceID addResource(Resource* resource);
    ResourceID createSawtoothWaveform();

    float* getWaveformBuffer(ResourceID id);


private:
    std::vector<Resource*> resources;
    ResourceID nextId;

    ResourceID getNextId(){ return nextId++; }
};
}