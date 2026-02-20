#pragma once

#include <vector>

namespace prs {

namespace SignalProcessing {

void normalize(std::vector<float>& signal, float headroom = 0.95f);

std::vector<float> convolve(const std::vector<float>& signal,
                            const std::vector<float>& impulse);

std::vector<float> fftConvolve(const std::vector<float>& signal,
                               const std::vector<float>& impulse);

} // namespace SignalProcessing

} // namespace prs
