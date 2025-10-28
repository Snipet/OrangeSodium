#include "delay_line.h"
#include "interpolation.h"
#include <cmath>
#include <cstring>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace OrangeSodium {

DelayLine::DelayLine(size_t max_delay_samples)
    : max_delay(max_delay_samples),
      write_index(0),
      delay_samples(max_delay_samples),
      mod_phase(0.0f),
      mod_amount(0.0f) {
    buffer = new float[max_delay];
    std::memset(buffer, 0, max_delay * sizeof(float));
}

DelayLine::~DelayLine() {
    if (buffer) {
        delete[] buffer;
        buffer = nullptr;
    }
}

void DelayLine::setDelayTime(size_t delay_samples) {
    // Clamp to valid range
    this->delay_samples = std::min(delay_samples, max_delay);
}

void DelayLine::setMaxDelayTime(size_t max_delay_samples) {
    // Reallocate buffer with new size
    if (buffer) {
        delete[] buffer;
    }
    max_delay = max_delay_samples;
    buffer = new float[max_delay];
    std::memset(buffer, 0, max_delay * sizeof(float));

    // Reset state
    write_index = 0;
    delay_samples = std::min(delay_samples, max_delay);
    mod_phase = 0.0f;
}

float DelayLine::tick(float input_sample) {
    // Write input to circular buffer
    buffer[write_index] = input_sample;

    // Calculate modulated delay time
    // LFO frequency: ~1 Hz at 44.1kHz (adjustable by scaling mod_phase increment)
    const float lfo_freq_hz = 1.5f;  // LFO frequency
    const float lfo_increment = 2.0f * static_cast<float>(M_PI) * lfo_freq_hz / sample_rate;

    // Generate sine LFO in range [-1, 1]
    const float lfo_value = std::sin(mod_phase);

    // Apply modulation to delay time
    // mod_amount is in samples, scales the LFO output
    const float modulated_delay = static_cast<float>(delay_samples) + (lfo_value * mod_amount);

    // Clamp modulated delay to valid range
    const float clamped_delay = std::max(1.0f, std::min(modulated_delay, static_cast<float>(max_delay - 1)));

    // Calculate read position (fractional) and read from circular buffer with interpolation
    float read_pos = static_cast<float>(write_index) - clamped_delay;
    const float output_sample = readCircularBufferLinear(buffer, max_delay, read_pos);

    // Advance write index (circular buffer)
    write_index = (write_index + 1) % max_delay;

    // Advance modulation phase
    mod_phase += lfo_increment;

    // Wrap phase to [0, 2*pi] to prevent overflow
    if (mod_phase >= 2.0f * static_cast<float>(M_PI)) {
        mod_phase -= 2.0f * static_cast<float>(M_PI);
    }

    return output_sample;
}

} // namespace OrangeSodium
