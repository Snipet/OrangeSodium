// Abstraction of audio and mod signals
#pragma once
#include <cstddef>

namespace OrangeSodium{

// Forward declaration for ObjectID type
using ObjectID = unsigned int;

class SignalBuffer{
public:

    enum class EType{
        kAudio = 0,
        kMod,
    };

    SignalBuffer(EType type, size_t n_frames, size_t n_channels);
    ~SignalBuffer();


    EType getType() noexcept { return type; }
    float* getChannel(size_t channel) noexcept { return (channel < n_channels) ? buffer[channel] : nullptr; }
    void setBufferId(size_t channel, unsigned int id) { if (channel < n_channels) buffer_ids[channel] = id; }

    size_t getChannelLength(size_t channel) noexcept { return (channel < n_channels) ? channel_lengths[channel] : 0; }
    size_t getChannelDivision(size_t channel) noexcept { return (channel < n_channels) ? channel_divisions[channel] : 1; }
    size_t getNumChannels() const noexcept { return n_channels; }
    void assignExistingBuffer(size_t channel, float* data, size_t length, size_t division, ObjectID id);
    ObjectID getId() const noexcept { return id; }
    void setId(ObjectID new_id) noexcept { id = new_id; }
    ObjectID getBufferId(size_t channel) noexcept { return (channel < n_channels) ? buffer_ids[channel] : 0; }
    void setChannelDivision(size_t channel, size_t division);
    void setConstantValue(size_t channel, float value);

    //void setChannelFromExistingBuffer(size_t channel, float* data, size_t length, size_t division, ObjectID id);

    /// @brief Zero out all buffer data
    void zeroOut();


    void resize(size_t* num_channels);
    void resize(size_t n_channels, size_t n_frames);

    /// @brief Set the buffer for a specific channel
    /// @param channel The channel index
    /// @param length The length of the buffer (in samples)
    /// @param division The division of the buffer (in samples)
    /// @param id The ID of the buffer source
    void setChannel(size_t channel, size_t length, size_t division, ObjectID id);

private:
    float** buffer;
    //bool* is_from_other_buffer;
    ObjectID* buffer_ids;     //IDs that point to the source of each channel (e.g. which oscillator, filter, effect, modulation producer, etc)
    size_t* channel_lengths;  // Length of each channel (in samples)
    size_t* channel_divisions; // For modulation buffers, this indicates how many samples to skip. For example, a division of 4 means the buffer is at 1/4 the sample rate of audio
    size_t n_channels;        // Number of channels
    EType type;
    ObjectID id;
};

}