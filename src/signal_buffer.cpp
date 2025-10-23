#include "signal_buffer.h"
#include "utilities.h"
#include <cstring>

namespace OrangeSodium {

SignalBuffer::SignalBuffer(EType type, size_t n_frames, size_t num_channels)
    : buffer(nullptr),
      buffer_ids(nullptr),
      channel_lengths(nullptr),
      channel_divisions(nullptr),
      n_channels(num_channels),
      type(type) {

    if (n_channels > 0) {
        buffer = new float*[n_channels];
        buffer_ids = new ObjectID[n_channels];
        channel_lengths = new size_t[n_channels];
        channel_divisions = new size_t[n_channels];

        for (size_t i = 0; i < n_channels; ++i) {
            if (n_frames > 0) {
                buffer[i] = new float[n_frames];
                std::memset(buffer[i], 0, n_frames * sizeof(float));
            } else {
                buffer[i] = nullptr;
            }
            buffer_ids[i] = 0;
            channel_lengths[i] = n_frames;
            channel_divisions[i] = 1; // Default division of 1 (no downsampling)
        }
    }
}

SignalBuffer::~SignalBuffer() {
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

void SignalBuffer::resize(size_t* num_channels) {
    if (!num_channels) {
        return;
    }

    size_t new_n_channels = *num_channels;

    // Allocate new arrays
    float** new_buffer = nullptr;
    ObjectID* new_buffer_ids = nullptr;
    size_t* new_channel_lengths = nullptr;
    size_t* new_channel_divisions = nullptr;

    if (new_n_channels > 0) {
        new_buffer = new float*[new_n_channels];
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

void SignalBuffer::setChannel(size_t channel, size_t length, size_t division, ObjectID id) {
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
        buffer[channel] = new float[length];
        std::memset(buffer[channel], 0, length * sizeof(float));
    }

    // Update metadata
    channel_lengths[channel] = length;
    channel_divisions[channel] = (division > 0) ? division : 1;
    buffer_ids[channel] = id;
}

void SignalBuffer::assignExistingBuffer(size_t channel, float* data, size_t length, size_t division, ObjectID id) {
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

void SignalBuffer::zeroOut() {
    if (!buffer) {
        return;
    }
    for (size_t i = 0; i < n_channels; ++i) {
        if (buffer[i]) {
            std::memset(buffer[i], 0, channel_lengths[i] * sizeof(float));
        }
    }
}

void SignalBuffer::setConstantValue(size_t channel, float value) {
    if (channel < n_channels && buffer[channel]) {
        for (size_t i = 0; i < channel_lengths[channel]; ++i) {
            buffer[channel][i] = value;
        }
    }
}

void SignalBuffer::resize(size_t n_channels, size_t n_frames) {
    size_t* num_channels = &n_channels;
    resize(num_channels);

    for (size_t i = 0; i < n_channels; ++i) {
        // Delete old buffer if it exists
        if (buffer[i]) {
            delete[] buffer[i];
        }

        // Allocate new buffer
        if (n_frames > 0) {
            buffer[i] = new float[n_frames];
            std::memset(buffer[i], 0, n_frames * sizeof(float));
        } else {
            buffer[i] = nullptr;
        }

        channel_lengths[i] = n_frames;
        channel_divisions[i] = 1; // Reset division to default
    }
}

}
