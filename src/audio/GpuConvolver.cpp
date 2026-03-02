#include "GpuConvolver.h"

#include <QDebug>

namespace prs {

GpuConvolver::GpuConvolver() = default;
GpuConvolver::~GpuConvolver() = default;

std::vector<float> GpuConvolver::fftConvolve(const std::vector<float>& signal,
                                             const std::vector<float>& impulse) {
    // For now, rely on the existing CPU implementation. This keeps numerical
    // behavior identical while leaving a well-defined hook for a future GPU
    // FFT/convolution backend.
    qWarning() << "GpuConvolver::fftConvolve using CPU fallback; GPU FFT not yet implemented.";
    return SignalProcessing::fftConvolve(signal, impulse);
}

} // namespace prs

