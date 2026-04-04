#pragma once

#include "core/Material.h"

#include <array>
#include <vector>

namespace prs {

namespace SignalProcessing {

void normalize(std::vector<float>& signal, float headroom = 0.95f);

std::vector<float> convolve(const std::vector<float>& signal, const std::vector<float>& impulse);

std::vector<float> fftConvolve(const std::vector<float>& signal, const std::vector<float>& impulse);

std::vector<float> bandpassFilter(const std::vector<float>& signal, int sampleRate, float centerFreq);

std::vector<float> combineMultibandRIR(const std::array<std::vector<float>, NUM_FREQ_BANDS>& bandRIRs, int sampleRate);

} // namespace SignalProcessing

} // namespace prs
