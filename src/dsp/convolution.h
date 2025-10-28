#pragma once
#include "../simd.h"

// Efficient convolution using SIMD
namespace OrangeSodium{

class Convolver {
public:
    Convolver(float* ir_buffer, size_t ir_length);
    ~Convolver();

    void setImpulseResponse(float* ir_buffer, size_t ir_length);
    inline float tick(float input);

private:
    float* impulse_response;
    float* history_buffer;
    size_t ir_length;
};

}