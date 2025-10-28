#pragma once
#include "oscillator.h"
#include "resource_manager.h"

namespace OrangeSodium{
class WaveformOscillator : public Oscillator {
public:
    WaveformOscillator(Context* context, ObjectID id, ResourceID waveform_id, size_t n_channels, float amplitude = 1.0f);
    ~WaveformOscillator() override;

    void processBlock(SignalBuffer* audio_inputs, SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_audio_frames) override;
    void onSampleRateChange(float new_sample_rate) override;

    void copyWaveformToPlaybackBuffer();
    void setWaveformResourceID(ResourceID resource_id) { waveform_resource_id = resource_id; }
    ResourceID getWaveformResourceID() const { return waveform_resource_id; }

private:
    ResourceID waveform_resource_id;
    float* phase; 
    float* playback_buffer; // Anti-aliased waveform data
    float* source_buffer;
    FFTManager* fft_manager;
    int* fft_tick;
    float* last_pitch;
    float* last_fft_pitch;
    int* bin_cutoff;
    float bins_allowed_above_nyquist;

    float getSampleFromWaveform(float phase_position);

    // Get the FFT bin cutoff for a given frequency. Used for anti-aliasing.
    int getBinCutoffForFrequency(float frequency);
    float getBinsAboveNyquistForFrequency(float frequency, int cutoff);
};
} // namespace OrangeSodium