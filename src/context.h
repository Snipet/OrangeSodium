//Synthesizer context shared across all objects
#pragma once
#include <string>
#include <iostream>
#include "resource_manager.h"
#include "dsp/fft.h"

enum EAudioQuality {
    kLowQuality = 0,
    kMediumQuality,
    kHighQuality
};

namespace OrangeSodium{

struct Context{
    std::string os_version;
    std::string os_build;
    std::string cpu_info;
    std::string simd_info;
    std::ostream* log_stream = &std::cout;
    //size_t block_size;
    double sample_rate; // Global sample rate
    int oversampling; // Global oversample rate
    EAudioQuality audio_quality = EAudioQuality::kHighQuality;
    bool is_playing = false; // Is the synthesizer currently playing audio?
    long long last_frame_time = 0; // Time in microseconds of the last audio frame processing. Used for performance monitoring.
    size_t n_voices = 0; //Number of voices in use
    size_t max_voices = 16; //Maximum number of voices allowed
    unsigned int next_object_id = 0; // Incrementing ID for all objects (oscillators, filters, effects, etc)
    size_t max_n_frames; // Number of frames per audio block

    ResourceManager* resource_manager;
    FFTManager* waveform_fft_manager;

    unsigned int getNextObjectID() { return next_object_id++; }
};

}