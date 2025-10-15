#include "voice.h"
#include "oscillators/sine_osc.h"

namespace OrangeSodium{

template <typename T>
Voice<T>::Voice(Context* context) : m_context(context)
{
    voice_master_audio_buffer = new SignalBuffer<T>(SignalBuffer<T>::EType::kAudio, context->n_frames, 2); // Default to stereo
}

template <typename T>
Voice<T>::~Voice()
{
    // Clean up oscillators
    for (auto* osc : oscillators) {
        delete osc;
    }
    // Clean up filters
    for (auto* filter : filters) {
        delete filter;
    }
    // Clean up buffers
    for (auto* buffer : audio_buffers) {
        delete buffer;
    }
    for (auto* buffer : mod_buffers) {
        delete buffer;
    }
}

template <typename T>
ObjectID Voice<T>::addSineOscillator(size_t n_channels) {
    ObjectID id = m_context->getNextObjectID();
    SineOscillator<T>* osc = new SineOscillator<T>(m_context, id, n_channels);
    oscillators.push_back(osc);
    oscillator_ids.push_back(id);
    return id;
}

template <typename T>
typename Voice<T>::EObjectType Voice<T>::getObjectType(ObjectID id) {
    // Check oscillators
    for (const auto& osc_id : oscillator_ids) {
        if (osc_id == id) {
            return EObjectType::kOscillator;
        }
    }
    // Check filters
    for (const auto& filter_id : filter_ids) {
        // Assuming Filter class has an 'id' member similar to Oscillator
        if (filter_id == id) {
            return EObjectType::kFilter;
        }
    }
    // Add checks for effects and modulators

    return EObjectType::kUndefined; // Not found
}

// Explicit template instantiations
template class Voice<float>;
template class Voice<double>;

}