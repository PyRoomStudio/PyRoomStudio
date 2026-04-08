#pragma once

#include "core/Types.h"

namespace prs {

enum class RenderMethod {
    RayTracing,
    DG_2D,
    DG_3D,
};

struct RenderOptions {
    RenderMethod method = RenderMethod::RayTracing;
    int maxOrder = DEFAULT_MAX_ORDER;
    int nRays = DEFAULT_N_RAYS;
    float scattering = DEFAULT_SCATTERING;
    bool airAbsorption = true;
    int sampleRate = DEFAULT_SAMPLE_RATE;
    int dgPolyOrder = 3;
    float dgMaxFrequency = 1000.0f;
};

} // namespace prs
