#include "signal_buffer.h"
#include <cstring>

namespace OrangeSodium {

template <typename T>
SignalBuffer<T>::SignalBuffer(EType type, size_t n_frames, size_t n_channels)
    : buffer(nullptr), buffer_ids(nullptr), n_frames(n_frames), n_channels(n_channels), type(type) {

    if (n_frames > 0 && n_channels > 0) {
        buffer = new T*[n_channels];
        buffer_ids = new ObjectID[n_channels];
        for (size_t i = 0; i < n_channels; ++i) {
            buffer[i] = new T[n_frames];
            std::memset(buffer[i], 0, n_frames * sizeof(T));
            buffer_ids[i] = 0;
        }
    }
}

template <typename T>
SignalBuffer<T>::~SignalBuffer() {
    if (buffer) {
        for (size_t i = 0; i < n_channels; ++i) {
            delete[] buffer[i];
        }
        delete[] buffer;
    }
    if (buffer_ids) {
        delete[] buffer_ids;
    }
}

template <typename T>
void SignalBuffer<T>::resize(size_t new_n_frames, size_t new_n_channels) {
    if (new_n_frames == n_frames && new_n_channels == n_channels) {
        return;
    }

    // Allocate new buffer
    T** new_buffer = nullptr;
    ObjectID* new_buffer_ids = nullptr;
    if (new_n_frames > 0 && new_n_channels > 0) {
        new_buffer = new T*[new_n_channels];
        new_buffer_ids = new ObjectID[new_n_channels];
        for (size_t i = 0; i < new_n_channels; ++i) {
            new_buffer[i] = new T[new_n_frames];
            std::memset(new_buffer[i], 0, new_n_frames * sizeof(T));
            new_buffer_ids[i] = 0;
        }

        // Copy existing data
        size_t copy_frames = (new_n_frames < n_frames) ? new_n_frames : n_frames;
        size_t copy_channels = (new_n_channels < n_channels) ? new_n_channels : n_channels;

        if (buffer) {
            for (size_t i = 0; i < copy_channels; ++i) {
                std::memcpy(new_buffer[i], buffer[i], copy_frames * sizeof(T));
                new_buffer_ids[i] = buffer_ids[i];
            }
        }
    }

    // Delete old buffer
    if (buffer) {
        for (size_t i = 0; i < n_channels; ++i) {
            delete[] buffer[i];
        }
        delete[] buffer;
    }
    if (buffer_ids) {
        delete[] buffer_ids;
    }

    // Update state
    buffer = new_buffer;
    buffer_ids = new_buffer_ids;
    n_frames = new_n_frames;
    n_channels = new_n_channels;
}

// Explicit template instantiations
template class SignalBuffer<float>;
template class SignalBuffer<double>;

}
