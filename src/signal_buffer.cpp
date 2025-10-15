#include "signal_buffer.h"
#include "utilities.h"
#include <cstring>

namespace OrangeSodium {

template <typename T>
SignalBuffer<T>::SignalBuffer(EType type, size_t n_frames, size_t num_channels)
    : buffer(nullptr),
      buffer_ids(nullptr),
      channel_lengths(nullptr),
      channel_divisions(nullptr),
      n_channels(num_channels),
      type(type) {

    if (n_channels > 0) {
        buffer = new T*[n_channels];
        buffer_ids = new ObjectID[n_channels];
        channel_lengths = new size_t[n_channels];
        channel_divisions = new size_t[n_channels];

        for (size_t i = 0; i < n_channels; ++i) {
            if (n_frames > 0) {
                buffer[i] = new T[n_frames];
                std::memset(buffer[i], 0, n_frames * sizeof(T));
            } else {
                buffer[i] = nullptr;
            }
            buffer_ids[i] = 0;
            channel_lengths[i] = n_frames;
            channel_divisions[i] = 1; // Default division of 1 (no downsampling)
        }
    }
}

template <typename T>
SignalBuffer<T>::~SignalBuffer() {
    if (buffer) {
        for (size_t i = 0; i < n_channels; ++i) {
            if (buffer[i]) {
                delete[] buffer[i];
            }
        }
        delete[] buffer;
    }
    if (buffer_ids) {
        delete[] buffer_ids;
    }
    if (channel_lengths) {
        delete[] channel_lengths;
    }
    if (channel_divisions) {
        delete[] channel_divisions;
    }
}

template <typename T>
void SignalBuffer<T>::resize(size_t* num_channels) {
    if (!num_channels) {
        return;
    }

    size_t new_n_channels = *num_channels;

    // Allocate new arrays
    T** new_buffer = nullptr;
    ObjectID* new_buffer_ids = nullptr;
    size_t* new_channel_lengths = nullptr;
    size_t* new_channel_divisions = nullptr;

    if (new_n_channels > 0) {
        new_buffer = new T*[new_n_channels];
        new_buffer_ids = new ObjectID[new_n_channels];
        new_channel_lengths = new size_t[new_n_channels];
        new_channel_divisions = new size_t[new_n_channels];

        for (size_t i = 0; i < new_n_channels; ++i) {
            new_buffer[i] = nullptr;
            new_buffer_ids[i] = 0;
            new_channel_lengths[i] = 0;
            new_channel_divisions[i] = 1;
        }
    }

    // Delete old arrays and individual channel buffers
    if (buffer) {
        for (size_t i = 0; i < n_channels; ++i) {
            if (buffer[i]) {
                delete[] buffer[i];
            }
        }
        delete[] buffer;
    }
    if (buffer_ids) {
        delete[] buffer_ids;
    }
    if (channel_lengths) {
        delete[] channel_lengths;
    }
    if (channel_divisions) {
        delete[] channel_divisions;
    }

    // Update state
    buffer = new_buffer;
    buffer_ids = new_buffer_ids;
    channel_lengths = new_channel_lengths;
    channel_divisions = new_channel_divisions;
    n_channels = new_n_channels;
}

template <typename T>
void SignalBuffer<T>::setChannel(size_t channel, size_t length, size_t division, ObjectID id) {
    if (!buffer || !buffer_ids || !channel_lengths || !channel_divisions || channel >= n_channels) {
        return;
    }

    // Delete old buffer for this channel if it exists
    if (buffer[channel]) {
        delete[] buffer[channel];
        buffer[channel] = nullptr;
    }

    // Allocate new buffer
    if (length > 0) {
        buffer[channel] = new T[length];
        std::memset(buffer[channel], 0, length * sizeof(T));
    }

    // Update metadata
    channel_lengths[channel] = length;
    channel_divisions[channel] = (division > 0) ? division : 1;
    buffer_ids[channel] = id;
}

template <typename T>
void SignalBuffer<T>::assignExistingBuffer(size_t channel, T* data, size_t length, size_t division, ObjectID id) {
    if (channel < n_channels) {
        if (buffer[channel]) {
            delete[] buffer[channel];
        }
        buffer[channel] = data;
        channel_lengths[channel] = length;
        channel_divisions[channel] = division;
        buffer_ids[channel] = id;
    }
}

// Explicit template instantiations
template class SignalBuffer<float>;
template class SignalBuffer<double>;

}
