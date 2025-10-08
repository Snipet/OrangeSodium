//Template filter class

#include "context.h"

namespace OrangeSodium{

template <typename T>
class Filter {
public:
    Filter(Context* context);
    ~Filter() = default;

    /// @brief Run the filter over nFrames.
    /// @param audio_inputs External audio source that may affect synthesis (FM, PM, etc.)
    /// @param mod_inputs External modulation inputs; inputs[0] is pitch (in hz); inputs[1] is amplitude [0, 1]; rest is implementation-specific
    /// @param outputs Audio output of oscillator
    /// @param config Denotes number of frames and channels for each buffer. Ordered by argument sequence and nframes, nchannels. Should be 6 size_t's
    void processBlock(T** audio_inputs, T** mod_inputs, T** outputs, size_t* config);

protected:
    Context* m_context;
};

}