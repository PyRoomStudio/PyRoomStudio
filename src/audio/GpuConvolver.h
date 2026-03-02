#pragma once

#include "SignalProcessing.h"

namespace prs {

/**
 * GPU-oriented implementation of IConvolver.
 *
 * The current implementation prepares a clear integration point for a future
 * GPU FFT/convolution path while delegating to the existing CPU
 * SignalProcessing::fftConvolve implementation.
 */
class GpuConvolver : public IConvolver {
public:
    GpuConvolver();
    ~GpuConvolver() override;

    std::vector<float> fftConvolve(const std::vector<float>& signal,
                                   const std::vector<float>& impulse) override;

private:
    struct GpuComplex {
        float real;
        float imag;
    };

    // A future GPU implementation can batch multiple convolutions by packing
    // time-domain or frequency-domain data into contiguous GPU buffers of
    // GpuComplex elements.
};

} // namespace prs

