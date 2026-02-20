#include "RayPicking.h"
#include <cmath>

namespace prs {
namespace RayPicking {

std::optional<float> rayTriangleIntersect(
    const Vec3f& rayOrigin, const Vec3f& rayDir, const Triangle& tri)
{
    constexpr float eps = 1e-8f;
    Vec3f edge1 = tri.v1 - tri.v0;
    Vec3f edge2 = tri.v2 - tri.v0;
    Vec3f h = rayDir.cross(edge2);
    float a = edge1.dot(h);

    if (a > -eps && a < eps) return std::nullopt;

    float f = 1.0f / a;
    Vec3f s = rayOrigin - tri.v0;
    float u = f * s.dot(h);
    if (u < 0.0f || u > 1.0f) return std::nullopt;

    Vec3f q = s.cross(edge1);
    float v = f * rayDir.dot(q);
    if (v < 0.0f || u + v > 1.0f) return std::nullopt;

    float t = f * edge2.dot(q);
    if (t > eps) return t;
    return std::nullopt;
}

std::optional<float> raySphereIntersect(
    const Vec3f& rayOrigin, const Vec3f& rayDir,
    const Vec3f& center, float radius)
{
    Vec3f oc = rayOrigin - center;
    float a = rayDir.dot(rayDir);
    float b = 2.0f * oc.dot(rayDir);
    float c = oc.dot(oc) - radius * radius;
    float disc = b * b - 4.0f * a * c;

    if (disc < 0.0f) return std::nullopt;

    float sqrtDisc = std::sqrt(disc);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);

    if (t1 > 0.0f) return t1;
    if (t2 > 0.0f) return t2;
    return std::nullopt;
}

} // namespace RayPicking
} // namespace prs
