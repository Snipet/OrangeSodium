// Abstraction of audio and mod signals
#pragma once
namespace OrangeSodium{

template <typename T>
class SignalBuffer{
public:

    enum class EType{
        kAudio = 0,
        kMod,
    }

    SignalBuffer(EType type, size_t n_frames, size_t n_channels);
    ~SignalBuffer();


    EType getType() noexcept { return type; }
    T* getChannel(size_t channel) noexcept { return (channel < n_channels) ? buffer[channel] : nullptr; }
    void setBufferId(size_t channel, unsigned int id) { if (channel < n_channels) buffer_ids[channel] = id; }

    void resize(size_t* num_channels);

    /// @brief Set the buffer for a specific channel
    /// @param channel The channel index
    /// @param length The length of the buffer (in samples)
    /// @param division The division of the buffer (in samples)
    /// @param id The ID of the buffer source
    void setChannel(size_t channel, size_t length, size_t division, ObjectID id);

private:
    T** buffer;
    ObjectID* buffer_ids;     //IDs that point to the source of each channel (e.g. which oscillator, filter, effect, modulation producer, etc)
    size_t* channel_lengths;  // Length of each channel (in samples)
    size_t* channel_divisions; // For modulation buffers, this indicates how many samples to skip. For example, a division of 4 means the buffer is at 1/4 the sample rate of audio
    EType type;
};

}