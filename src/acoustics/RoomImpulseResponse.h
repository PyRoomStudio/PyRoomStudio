#pragma once

#include "ImageSourceMethod.h"
#include "RayTracer.h"

#include <vector>

namespace prs {

class RoomImpulseResponse {
public:
    std::vector<float> compute(
        const std::vector<ImageSource>& imageSources,
        const std::vector<RayContribution>& rayContributions,
        int sampleRate);

    float duration() const { return duration_; }

private:
    float duration_ = 0.0f;
};

} // namespace prs
