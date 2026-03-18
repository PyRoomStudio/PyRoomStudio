#include "Material.h"
#include <cmath>
#include <algorithm>

namespace prs {

float Material::averageAbsorption() const {
    float sum = 0.0f;
    for (float a : absorption) sum += a;
    return sum / NUM_FREQ_BANDS;
}

float Material::absorptionAt(int freqHz) const {
    if (freqHz <= FREQ_BANDS[0])
        return absorption[0];
    if (freqHz >= FREQ_BANDS[NUM_FREQ_BANDS - 1])
        return absorption[NUM_FREQ_BANDS - 1];

    for (int i = 0; i < NUM_FREQ_BANDS - 1; ++i) {
        if (freqHz >= FREQ_BANDS[i] && freqHz <= FREQ_BANDS[i + 1]) {
            float logLow = std::log2(static_cast<float>(FREQ_BANDS[i]));
            float logHigh = std::log2(static_cast<float>(FREQ_BANDS[i + 1]));
            float logFreq = std::log2(static_cast<float>(freqHz));
            float t = (logFreq - logLow) / (logHigh - logLow);
            return absorption[i] * (1.0f - t) + absorption[i + 1] * t;
        }
    }
    return absorption[0];
}

} // namespace prs
