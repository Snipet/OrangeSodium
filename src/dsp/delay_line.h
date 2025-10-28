#pragma once

namespace OrangeSodium{

class DelayLine{
public:
    DelayLine(size_t max_delay_samples);
    ~DelayLine();

    /// @brief Set the delay time in samples
    /// @param delay_samples The delay time in samples
    void setDelayTime(size_t delay_samples);

    /// @brief Process a single sample through the delay line
    /// @param input_sample The input sample
    /// @return The delayed output sample
    float tick(float input_sample);

    /// @brief Set the modulation amount for modulated delay
    void setModulationAmount(float mod_amount) {
        this->mod_amount = mod_amount;
    }

    /// @brief Get the current modulation amount
    /// @return The current modulation amount
    float getModulationAmount() const {
        return mod_amount;
    }

    void reset() {
        write_index = 0;
        for (size_t i = 0; i < max_delay; ++i) {
            buffer[i] = 0.0f;
        }
        mod_phase = 0.0f;
    }

    /// @brief Set the max delay time in samples
    void setMaxDelayTime(size_t max_delay_samples);

    /// @brief Set the sample rate
    void setSampleRate(float rate) {
        sample_rate = rate;
    }

private:
    float* buffer;
    size_t max_delay;
    size_t write_index;
    size_t delay_samples;
    float sample_rate;

    float mod_phase; // Used for modulated delay using sine LFO
    float mod_amount; // Amount of modulation to apply

};

}