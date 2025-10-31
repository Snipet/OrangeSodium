#include "waveform_osc.h"
#include "constants.h"
namespace OrangeSodium {

WaveformOscillator::WaveformOscillator(Context* context, ObjectID id, ResourceID waveform_id, size_t n_channels, float amplitude)
    : Oscillator(context, id, n_channels, amplitude), waveform_resource_id(waveform_id), phase(nullptr), playback_buffer(nullptr) {
    fft_manager = context->waveform_fft_manager;
    // Initialize playback buffer
    playback_buffer = new float[WAVEFORM_STANDARD_LENGTH];
    for (size_t i = 0; i < WAVEFORM_STANDARD_LENGTH; ++i) {
        playback_buffer[i] = 0.0f;
    }

    source_buffer = m_context->resource_manager->getWaveformBuffer(waveform_resource_id);
    phase = new float[n_channels];
    fft_tick = new int[n_channels];
    last_pitch = new float[n_channels];
    last_fft_pitch = new float[n_channels];
    bin_cutoff = new int[n_channels];
    for (size_t c = 0; c < n_channels; ++c) {
        phase[c] = 0.0f;
        fft_tick[c] = 0;
        last_pitch[c] = 0.0f;
        last_fft_pitch[c] = 0.0f;
        bin_cutoff[c] = 0;
    }
    copyWaveformToPlaybackBuffer();
    bins_allowed_above_nyquist = 5.f;


    // Add modulation source names
    modulation_source_names.resize(0);
    modulation_source_names.push_back("pitch");
    modulation_source_names.push_back("amplitude");
}

WaveformOscillator::~WaveformOscillator() {
    if (playback_buffer) {
        delete[] playback_buffer;
        playback_buffer = nullptr;
    }
    if (phase) {
        delete[] phase;
        phase = nullptr;
    }
    if (fft_tick) {
        delete[] fft_tick;
        fft_tick = nullptr;
    }
    if (last_pitch) {
        delete[] last_pitch;
        last_pitch = nullptr;
    }
    if (last_fft_pitch) {
        delete[] last_fft_pitch;
        last_fft_pitch = nullptr;
    }
    if (bin_cutoff) {
        delete[] bin_cutoff;
        bin_cutoff = nullptr;
    }
}

void WaveformOscillator::onSampleRateChange(float new_sample_rate) {
    sample_rate = new_sample_rate;

}

void WaveformOscillator::copyWaveformToPlaybackBuffer(){
    if(waveform_resource_id == static_cast<ResourceID>(-1)){
        return;
    }
    ResourceManager* resource_manager = m_context->resource_manager;
    if(!resource_manager){
        return;
    }
    if(!source_buffer){
        return;
    }
    // Copy waveform data to playback buffer
    for(size_t i = 0; i < WAVEFORM_STANDARD_LENGTH; ++i){
        playback_buffer[i] = source_buffer[i];
    }

}

void WaveformOscillator::processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) {
    const size_t n_frames = n_audio_frames; // Will always be lower than or equal to context->max_n_frames

    // Get pitch buffer from mod_inputs
    float* pitch_buffer = mod_inputs->getChannel(static_cast<size_t>(EModChannel::kPitch));
    float* amplitude_buffer = mod_inputs->getChannel(static_cast<size_t>(EModChannel::kAmplitude));
    const size_t pitch_buffer_divisions = mod_inputs->getChannelDivision(static_cast<size_t>(EModChannel::kPitch));
    const size_t amplitude_buffer_divisions = mod_inputs->getChannelDivision(static_cast<size_t>(EModChannel::kAmplitude));
    for(size_t c = 0; c < n_channels; ++c) {
        float* out_buffer = outputs->getChannel(c);
        if (!out_buffer) {
            continue;
        }


        for (size_t i = 0; i < n_frames; ++i) {
            // Determine if we need to update the playback buffer based on pitch change
            // We need to update playback_buffer if the pitch has changed too far from last_fft_pitch


            const float pitch_hz = getHzFromMIDINote(pitch_buffer[(i + frame_offset) / pitch_buffer_divisions] + frequency_offset);
            if(fft_tick[c] >= 4){
                const float bins_above_nyq = getBinsAboveNyquistForFrequency(pitch_hz, bin_cutoff[c]);
                if(bins_above_nyq < 1.f || bins_above_nyq > bins_allowed_above_nyquist){
                    // We need to update the playback buffer so that its bins above nyquist is equal to bins_allowed_above_nyquist
                    bin_cutoff[c] = getBinCutoffForFrequency(pitch_hz) + static_cast<int>(bins_allowed_above_nyquist);
                    fft_manager->brickwallWaveform(source_buffer, playback_buffer, bin_cutoff[c]);
                }
                fft_tick[c] = 0;
            }
            const float pitch_norm = pitch_hz / (sample_rate);
            float phase_increment = pitch_norm;
            phase[c] += phase_increment;
            phase[c] = std::fmod(phase[c], 1.f);
            const float amp = amplitude + ((amplitude_buffer) ? amplitude_buffer[(i + frame_offset) / amplitude_buffer_divisions] : 0.0f);
            out_buffer[i + frame_offset] += amp * getSampleFromWaveform(phase[c]);

            fft_tick[c]++;
            last_pitch[c] = pitch_hz;
        }
    }
    frame_offset += n_frames;
}

float WaveformOscillator::getSampleFromWaveform(float phase_position) {
    // phase_position is in [0, 1)
    float index = phase_position * static_cast<float>(WAVEFORM_STANDARD_LENGTH);
    size_t index_int = static_cast<size_t>(index);
    float frac = index - static_cast<float>(index_int);
    size_t index_next = (index_int + 1) % WAVEFORM_STANDARD_LENGTH;

    // Linear interpolation
    float sample = (1.0f - frac) * playback_buffer[index_int] + frac * playback_buffer[index_next];
    return sample;
}

int WaveformOscillator::getBinCutoffForFrequency(float frequency) {
    float nyquist = sample_rate / 2.f;
    return static_cast<int>((nyquist / frequency)) + 1;
}

float WaveformOscillator::getBinsAboveNyquistForFrequency(float frequency, int cutoff) {
    float nyquist = sample_rate / 2.f;
    float max_frequency = static_cast<float>(cutoff - 1) * frequency;
    return (max_frequency - nyquist) / frequency;
}


} // namespace OrangeSodium