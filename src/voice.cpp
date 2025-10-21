#include "voice.h"
#include "oscillators/sine_osc.h"

namespace OrangeSodium{

Voice::Voice(Context* context) : m_context(context)
{
    voice_master_audio_buffer = new SignalBuffer(SignalBuffer::EType::kAudio, context->n_frames, 2); // Default to stereo
}

Voice::~Voice()
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

ObjectID Voice::addSineOscillator(size_t n_channels) {
    ObjectID id = m_context->getNextObjectID();
    SineOscillator* osc = new SineOscillator(m_context, id, n_channels);
    SignalBuffer* mod_buffer = new SignalBuffer(SignalBuffer::EType::kMod, m_context->n_frames, 2); // Sine oscillator uses 2 mod channels (pitch, amplitude)
    mod_buffer->setId(m_context->getNextObjectID());
    mod_buffer->setChannelDivision(0, 1); // Pitch channel at full sample rate
    mod_buffer->setChannelDivision(1, 1); // Amplitude channel at full sample rate
    mod_buffers.push_back(mod_buffer);
    osc->setModBuffer(mod_buffer);
    oscillators.push_back(osc);
    oscillator_ids.push_back(id);
    return id;
}

Voice::EObjectType Voice::getObjectType(ObjectID id) {
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
    
    // Check audio buffers
    for (const auto& buffer : audio_buffers) {
        if (buffer->getId() == id) {
            return EObjectType::kAudioBuffer; // Assuming audio buffers are considered effects here
        }
    }

    return EObjectType::kUndefined; // Not found
}

void Voice::resizeAudioBuffers(size_t n_frames) {
    for (auto* buffer : audio_buffers) {
        if (buffer->getType() == SignalBuffer::EType::kAudio) {
            for (size_t ch = 0; ch < buffer->getNumChannels(); ++ch) {
                buffer->setChannel(ch, n_frames, 1, buffer->getBufferId(ch));
            }
        }
    }
}

ObjectID Voice::addAudioBuffer(size_t n_frames, size_t n_channels) {
    ObjectID id = m_context->getNextObjectID();
    SignalBuffer* buffer = new SignalBuffer(SignalBuffer::EType::kAudio, n_frames, n_channels);
    buffer->setId(id);
    audio_buffers.push_back(buffer);
    return id;
}

void Voice::assignOscillatorAudioBuffer(ObjectID osc_id, ObjectID buffer_id) {
    // Find the oscillator
    for (size_t i = 0; i < oscillators.size(); ++i) {
        if (oscillator_ids[i] == osc_id) {
            // Find the buffer
            for (auto* buffer : audio_buffers) {
                if (buffer->getId() == buffer_id) {
                    oscillators[i]->setOutputBuffer(buffer);
                    return;
                }
            }
        }
    }
}

ObjectID Voice::getConnectedAudioBufferForOscillator(ObjectID osc_id) {
    // Find the oscillator
    for (size_t i = 0; i < oscillators.size(); ++i) {
        if (oscillator_ids[i] == osc_id) {
            SignalBuffer* output_buffer = oscillators[i]->getOutputBuffer();
            if (output_buffer) {
                return output_buffer->getId();
            } else {
                return -1; // No buffer assigned
            }
        }
    }
    return -1; // Oscillator not found
}

ErrorCode Voice::connectMasterAudioBufferToSource(ObjectID buffer_id) {
    // Find the buffer
    for (auto* buffer : audio_buffers) {
        if (buffer->getId() == buffer_id) {
            voice_master_audio_buffer_src_ptr = buffer;
            return ErrorCode::kNoError;
        }
    }

    return ErrorCode::kAudioBufferNotFound;
}

void Voice::processVoice(size_t n_audio_frames) {
    // First, zero out the audio buffers
    for(auto* buffer : audio_buffers){
        if(buffer->getType() == SignalBuffer::EType::kAudio){
            buffer->zeroOut();
        }
    }

    // Second, we need to set modulation inputs for each oscillator
    // For now, just set pitch and amplitude based on current_midi_note
    const float pitch_hz = getHzFromMIDINote(static_cast<int>(current_midi_note));
    for(auto* osc : oscillators){
        SignalBuffer* mod_buffer = osc->getModBuffer();
        if(mod_buffer){
            mod_buffer->setConstantValue(static_cast<size_t>(Oscillator::EModChannel::kPitch), pitch_hz);
            mod_buffer->setConstantValue(static_cast<size_t>(Oscillator::EModChannel::kAmplitude), 1.0f);
        }
    }

    for(auto* osc : oscillators){
        osc->processBlock(nullptr, osc->getModBuffer(), osc->getOutputBuffer(), n_audio_frames); // For now, no inputs
    }

    // Copy data from source buffer to master audio buffer
    if(voice_master_audio_buffer_src_ptr && voice_master_audio_buffer){
        size_t n_channels = voice_master_audio_buffer->getNumChannels();
        size_t n_frames = m_context->n_frames;
        for(size_t c = 0; c < n_channels; ++c){
            float* src_channel = voice_master_audio_buffer_src_ptr->getChannel(c);
            float* dest_channel = voice_master_audio_buffer->getChannel(c);
            if(src_channel && dest_channel){
                std::memcpy(dest_channel, src_channel, n_frames * sizeof(float));
            }
        }
    }
}


}