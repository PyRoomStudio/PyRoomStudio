#include "RoomImpulseResponse.h"

#include <algorithm>
#include <cmath>

namespace prs {

std::array<std::vector<float>, NUM_FREQ_BANDS>
RoomImpulseResponse::computeMultiband(const std::vector<ImageSource>& imageSources,
                                      const std::vector<RayContribution>& rayContributions, int sampleRate) {
    float maxDelay = 0.0f;
    for (auto& is : imageSources)
        maxDelay = std::max(maxDelay, is.delay);
    for (auto& rc : rayContributions)
        maxDelay = std::max(maxDelay, rc.delay);

    maxDelay += 0.05f;
    duration_ = maxDelay;

    int numSamples = static_cast<int>(maxDelay * sampleRate) + 1;
    if (numSamples <= 0)
        numSamples = 1;

    std::array<std::vector<float>, NUM_FREQ_BANDS> rirs;
    for (auto& rir : rirs)
        rir.assign(numSamples, 0.0f);

    for (auto& is : imageSources) {
        int sampleIdx = static_cast<int>(is.delay * sampleRate);
        if (sampleIdx >= 0 && sampleIdx < numSamples) {
            for (int b = 0; b < NUM_FREQ_BANDS; ++b)
                rirs[b][sampleIdx] += is.attenuation[b];
        }
    }

    float rayScale = 1.0f / std::max(1.0f, static_cast<float>(rayContributions.size()) * 0.001f);
    for (auto& rc : rayContributions) {
        int sampleIdx = static_cast<int>(rc.delay * sampleRate);
        if (sampleIdx >= 0 && sampleIdx < numSamples) {
            for (int b = 0; b < NUM_FREQ_BANDS; ++b)
                rirs[b][sampleIdx] += rc.energy[b] * rayScale;
        }
    }

    return rirs;
}

} // namespace prs
