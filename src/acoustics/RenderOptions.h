#pragma once

#include "core/Types.h"

#include <QString>

namespace prs {

enum class RenderMethod {
    RayTracing,
    DG_2D,
    DG_3D,
};

// Controls how listener output is rendered and exported.
//
// Mono    — single-channel room impulse response (DG path default).
// Stereo  — physical ear-offset stereo without HRTF filtering (existing ray-tracing path).
// Binaural — HRTF-convolved stereo with head-orientation-aware direction lookup.
enum class AudioOutputMode {
    Mono,
    Stereo,
    Binaural,
};

struct RenderOptions {
    RenderMethod method = RenderMethod::RayTracing;
    AudioOutputMode outputMode = AudioOutputMode::Stereo;
    QString hrtfDatasetPath;

    int maxOrder = DEFAULT_MAX_ORDER;
    int nRays = DEFAULT_N_RAYS;
    float scattering = DEFAULT_SCATTERING;
    bool airAbsorption = true;
    int sampleRate = DEFAULT_SAMPLE_RATE;
    int dgPolyOrder = 3;
    float dgMaxFrequency = 1000.0f;
};

} // namespace prs
