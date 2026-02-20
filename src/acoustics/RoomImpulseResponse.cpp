#include "RoomImpulseResponse.h"
#include <cmath>
#include <algorithm>

namespace prs {

std::vector<float> RoomImpulseResponse::compute(
    const std::vector<ImageSource>& imageSources,
    const std::vector<RayContribution>& rayContributions,
    int sampleRate)
{
    // Find maximum delay to determine RIR length
    float maxDelay = 0.0f;
    for (auto& is : imageSources)
        maxDelay = std::max(maxDelay, is.delay);
    for (auto& rc : rayContributions)
        maxDelay = std::max(maxDelay, rc.delay);

    // Add margin
    maxDelay += 0.05f;
    duration_ = maxDelay;

    int numSamples = static_cast<int>(maxDelay * sampleRate) + 1;
    if (numSamples <= 0) numSamples = 1;

    std::vector<float> rir(numSamples, 0.0f);

    // Add image source contributions (early reflections)
    for (auto& is : imageSources) {
        int sampleIdx = static_cast<int>(is.delay * sampleRate);
        if (sampleIdx >= 0 && sampleIdx < numSamples) {
            rir[sampleIdx] += is.attenuation;
        }
    }

    // Add ray tracing contributions (late reverb)
    float rayScale = 1.0f / std::max(1.0f, static_cast<float>(rayContributions.size()) * 0.001f);
    for (auto& rc : rayContributions) {
        int sampleIdx = static_cast<int>(rc.delay * sampleRate);
        if (sampleIdx >= 0 && sampleIdx < numSamples) {
            rir[sampleIdx] += rc.energy * rayScale;
        }
    }

    return rir;
}

} // namespace prs
