#pragma once

#include "core/Material.h"
#include "ImageSourceMethod.h"
#include "RayTracer.h"

#include <array>
#include <vector>

namespace prs {

class RoomImpulseResponse {
  public:
    std::array<std::vector<float>, NUM_FREQ_BANDS>
    computeMultiband(const std::vector<ImageSource>& imageSources, const std::vector<RayContribution>& rayContributions,
                     int sampleRate);

    float duration() const { return duration_; }

  private:
    float duration_ = 0.0f;
};

} // namespace prs
