#include "../modulator_producer.h"
#include <fstream>
/*
* Basic ADSR Envelope Modulation Producer
* Generates an ADSR envelope based on retrigger input
* Modulation inputs: 
*                  [0] - Attack time (seconds)
*                  [1] - Decay time (seconds)
*                  [2] - Sustain level (0.0 to 1.0)
*                  [3] - Release time (seconds)
* Outputs:
*                  [0] - Envelope output (0.0 to 1.0)
*/

namespace OrangeSodium {

class BasicEnvelope : public ModulationProducer {
public:
    BasicEnvelope(Context* context, ObjectID id);
    BasicEnvelope(Context* context, ObjectID id, float attack, float decay, float sustain, float release);
    ~BasicEnvelope();

    void processBlock(SignalBuffer* mod_inputs, SignalBuffer* outputs, size_t n_frames) override;
    void onSampleRateChange(float new_sample_rate) override;
    void onRetrigger() override {
        retrigger();
    }
    void onRelease() override {
        current_stage = EStage::kRelease;
    }
    void retrigger() {
        is_retriggered = true;
        current_stage = EStage::kAttack;
        state = 0.0f;
    }

private:
    float attack_time; // In seconds
    float decay_time; // In seconds
    float sustain_level; // 0.0 to 1.0
    float release_time; // In seconds
    bool is_retriggered = false;
    float release_level; // level at time of release

    float state = 0.0f;

    enum class EStage {
        kIdle = 0,
        kAttack,
        kDecay,
        kSustain,
        kRelease
    };

    EStage current_stage = EStage::kIdle;
};

} // namespace OrangeSodium
