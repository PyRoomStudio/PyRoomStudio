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

} // namespace RayPicking

} // namespace prs
