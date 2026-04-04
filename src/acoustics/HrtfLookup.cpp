#include "HrtfLookup.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace prs {

HeadFrameDirection toHeadFrame(const Vec3f& sourcePos, const Vec3f& listenerPos,
                               const Vec3f& listenerForward, const Vec3f& worldUp) {
    const Vec3f direction = (sourcePos - listenerPos).normalized();
    const Vec3f forward = listenerForward.normalized();

    // right = forward × worldUp  (points to the listener's right in Y-up scenes)
    const Vec3f right = forward.cross(worldUp).normalized();
    // recompute up orthogonal to both forward and right
    const Vec3f up = right.cross(forward);

    const float azimuthRad = std::atan2(direction.dot(right), direction.dot(forward));
    const float elevationRad = std::asin(std::clamp(direction.dot(up), -1.0f, 1.0f));

    constexpr float kToDeg = 180.0f / static_cast<float>(M_PI);
    return {azimuthRad * kToDeg, elevationRad * kToDeg};
}

HrtfSample lookupWithFallback(const HrtfDataset& dataset, float azimuthDeg, float elevationDeg) {
    auto result = dataset.lookup(azimuthDeg, elevationDeg);
    if (result.has_value())
        return *result;

    // Dataset is empty — return a unit-impulse passthrough so the render path
    // never has to handle a missing sample.
    const int filterLength = std::max(1, dataset.metadata().filterLength);
    HrtfSample fallback;
    fallback.azimuthDeg = azimuthDeg;
    fallback.elevationDeg = elevationDeg;
    fallback.leftDelaySamples = 0;
    fallback.rightDelaySamples = 0;
    fallback.left.assign(static_cast<size_t>(filterLength), 0.0f);
    fallback.right.assign(static_cast<size_t>(filterLength), 0.0f);
    fallback.left[0] = 1.0f;
    fallback.right[0] = 1.0f;
    return fallback;
}

} // namespace prs
