#pragma once

#include "core/Types.h"
#include <optional>

namespace prs {

namespace RayPicking {

std::optional<float> rayTriangleIntersect(
    const Vec3f& rayOrigin, const Vec3f& rayDir, const Triangle& tri);

std::optional<float> raySphereIntersect(
    const Vec3f& rayOrigin, const Vec3f& rayDir,
    const Vec3f& center, float radius);

/** Returns true if segment [a,b] intersects the sphere (center, radius) at any point strictly between a and b. */
bool segmentPassesThroughSphere(const Vec3f& a, const Vec3f& b,
                                const Vec3f& center, float radius);

} // namespace RayPicking

} // namespace prs
