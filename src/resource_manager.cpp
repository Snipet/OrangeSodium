#include "resource_manager.h"
#include "constants.h"

namespace OrangeSodium{

Resource::Resource(EType type) : type_(type){}
Resource::~Resource() {}

WaveformResource::WaveformResource(Resource::EType type, size_t length) : Resource(type), data(nullptr), length(length) {
    // Initialize data
    data = new float[length];
    for (size_t i = 0; i < length; ++i) {
        data[i] = 0.0f;
    }
}
WaveformResource::~WaveformResource() {
    delete[] data;
    data = nullptr;
}

void WaveformResource::createSawtooth() {
    for (size_t i = 0; i < length; ++i) {
        data[i] = static_cast<float>(i) / static_cast<float>(length) * 2.0f - 1.0f; // Sawtooth from -1.0 to 1.0
    }
}

ResourceManager::ResourceManager() : nextId(0) {}
ResourceManager::~ResourceManager() {
    for (Resource* res : resources) {
        delete res;
    }
    resources.clear();
}
ResourceID ResourceManager::addResource(Resource* resource) {
    resource->setId(getNextId());
    resources.push_back(resource);
    return resource->getId();
}

ResourceID ResourceManager::createSawtoothWaveform() {
    size_t length = WAVEFORM_STANDARD_LENGTH; // Standard waveform length
    WaveformResource* waveform = new WaveformResource(Resource::EType::kWaveform, length);
    waveform->createSawtooth();
    return addResource(waveform);
}

float* ResourceManager::getWaveformBuffer(ResourceID id) {
    for (Resource* res : resources) {
        if (res->getId() == id && res->getType() == Resource::EType::kWaveform) {
            WaveformResource* waveform = static_cast<WaveformResource*>(res);
            return waveform->getData();
        }
    }
    return nullptr; // Not found
}

} // namespace OrangeSodium