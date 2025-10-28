#include "fft.h"



namespace OrangeSodium{

FFTManager::FFTManager(unsigned int N){
    initialize(N);
}


bool FFTManager::initialize(int order){
    // ---- query required byte sizes ----
    int sizeSpec = 0, sizeInit = 0, sizeBuf = 0;
    IppStatus st = ippsFFTGetSize_C_32fc(
        order,
        IPP_FFT_DIV_INV_BY_N,      // inverse scales by 1/N (common audio convention)
        hint_,                     // ippAlgHintAccurate by default
        &sizeSpec, &sizeInit, &sizeBuf
    );
    if (st != ippStsNoErr) return false;

    // ---- allocate memory (use temporaries first; only commit on success) ----
    std::unique_ptr<Ipp8u, IppFree> specMem( sizeSpec ? ippsMalloc_8u(sizeSpec) : nullptr );
    std::unique_ptr<Ipp8u, IppFree> initMem( sizeInit ? ippsMalloc_8u(sizeInit) : nullptr );
    std::unique_ptr<Ipp8u, IppFree> buf    ( sizeBuf  ? ippsMalloc_8u(sizeBuf)  : nullptr );

    if ((sizeSpec && !specMem) || (sizeInit && !initMem) || (sizeBuf && !buf))
        return false; // allocation failure

    // ---- build spec (spec_ points inside specMem) ----
    IppsFFTSpec_C_32fc* specTmp = nullptr;
    st = ippsFFTInit_C_32fc(
        &specTmp,
        order,
        IPP_FFT_DIV_INV_BY_N,
        hint_,
        specMem.get(),
        initMem.get()
    );
    if (st != ippStsNoErr) return false;

    // ---- commit to members (re-init safe: old memory freed by unique_ptrs) ----
    spec_     = specTmp;
    spec_mem_ = std::move(specMem);
    buf_      = std::move(buf);
    fft_size_ = 1 << order; // 2^order

    // ---- allocate complex buffers for brickwallWaveform ----
    complex_buf1_.reset(ippsMalloc_32fc(fft_size_));
    complex_buf2_.reset(ippsMalloc_32fc(fft_size_));

    if (!complex_buf1_ || !complex_buf2_) {
        return false; // allocation failure
    }

    // initMem is temporary and frees automatically here
    return true;
}

void FFTManager::brickwallWaveform(const float* input, float* output, size_t bin_cutoff) {
    if (!spec_ || fft_size_ <= 0 || !complex_buf1_ || !complex_buf2_) {
        // FFT not initialized, just copy input to output
        //Throw error
        std::runtime_error("FFTManager not initialized properly.");
        return;
        for (int i = 0; i < fft_size_; i++) {
            output[i] = input[i];
        }
        return;
    }

    // Clamp bin_cutoff to valid range
    if (bin_cutoff > fft_size_ / 2) {
        bin_cutoff = fft_size_ / 2;
    }

    Ipp32fc* complex_in = complex_buf1_.get();
    Ipp32fc* complex_out = complex_buf2_.get();

    // Convert real input to complex (imaginary parts = 0)
    for (int i = 0; i < fft_size_; i++) {
        complex_in[i].re = input[i];
        complex_in[i].im = 0.0f;
    }

    // Forward FFT: time domain -> frequency domain
    ippsFFTFwd_CToC_32fc(
        complex_in,
        complex_out,
        spec_,
        buf_.get()
    );

    // Apply brickwall filter: zero out bins above cutoff
    // For real signals, the FFT is symmetric, so we need to handle both positive and negative frequencies
    // Bins 0 to bin_cutoff: keep (positive frequencies)
    // Bins bin_cutoff+1 to fft_size-bin_cutoff-1: zero out (high frequencies)
    // Bins fft_size-bin_cutoff to fft_size-1: keep (negative frequencies)

    for (size_t i = bin_cutoff + 1; i < fft_size_ - bin_cutoff; i++) {
        complex_out[i].re = 0.0f;
        complex_out[i].im = 0.0f;
    }

    // Inverse FFT: frequency domain -> time domain
    ippsFFTInv_CToC_32fc(
        complex_out,
        complex_in,  // reuse complex_in for output
        spec_,
        buf_.get()
    );

    // Extract real part (imaginary part should be negligible)
    for (int i = 0; i < fft_size_; i++) {
        output[i] = complex_in[i].re;
    }
}

FFTManager::~FFTManager() {
    // unique_ptrs automatically free memory
}

} // namespace OrangeSodium