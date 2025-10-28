#pragma once

// FFT wrapper for Intel IPP FFT

#include "ipp/ipps.h"
#include <iostream>
#include <memory>


namespace OrangeSodium{

struct IppFree {
    void operator()(Ipp8u* p) const noexcept { if (p) ippsFree(p); }
};

struct IppFreeCplx {
    void operator()(Ipp32fc* p) const noexcept { if (p) ippsFree(p); }
};

class FFTManager{
public:
    FFTManager(unsigned int N);
    ~FFTManager();

    void brickwallWaveform(const float* input, float* output, size_t bin_cutoff);

private:
    IppHintAlgorithm hint_{ippAlgHintAccurate};
    // IPP objects
    IppsFFTSpec_C_32fc* spec_;                // points INTO spec_mem_
    std::unique_ptr<Ipp8u, IppFree> spec_mem_; // owns spec storage
    std::unique_ptr<Ipp8u, IppFree> buf_;      // scratch for calls
    int fft_size_{0};                          // FFT size (2^order)

    // Pre-allocated complex buffers for brickwallWaveform
    std::unique_ptr<Ipp32fc, IppFreeCplx> complex_buf1_; // reusable buffer
    std::unique_ptr<Ipp32fc, IppFreeCplx> complex_buf2_; // reusable buffer

    bool initialize(int order);
};

}