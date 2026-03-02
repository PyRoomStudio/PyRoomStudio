#include "ImageSourceMethod.h"
#include "rendering/RayPicking.h"
#include <cmath>

namespace prs {

// ── Original per-triangle interface (kept for backwards compat) ─────────────

std::vector<ImageSource> ImageSourceMethod::compute(
    const Vec3f& sourcePos,
    const Vec3f& listenerPos,
    const std::vector<Wall>& walls,
    int maxOrder)
{
    std::vector<ImageSource> results;

    if (isVisible(sourcePos, listenerPos, walls)) {
        float dist = (listenerPos - sourcePos).norm();
        ImageSource direct;
        direct.position    = sourcePos;
        direct.attenuation = 1.0f / std::max(dist, 0.001f);
        direct.delay       = dist / SPEED_OF_SOUND;
        direct.order       = 0;
        results.push_back(direct);
    }

    generateImageSources(sourcePos, walls, 1, maxOrder, 1.0f, {}, results);

    std::vector<ImageSource> valid;
    for (auto& is : results) {
        if (is.order == 0) {
            valid.push_back(is);
            continue;
        }
        if (isVisible(is.position, listenerPos, walls)) {
            float dist = (listenerPos - is.position).norm();
            is.attenuation /= std::max(dist, 0.001f);
            is.delay = dist / SPEED_OF_SOUND;
            valid.push_back(is);
        }
    }

    return valid;
}

void ImageSourceMethod::generateImageSources(
    const Vec3f& source,
    const std::vector<Wall>& walls,
    int order, int maxOrder,
    float attenuation,
    const std::vector<int>& path,
    std::vector<ImageSource>& results)
{
    if (order > maxOrder) return;

    for (int wi = 0; wi < static_cast<int>(walls.size()); ++wi) {
        if (!path.empty() && path.back() == wi) continue;

        Vec3f reflected = walls[wi].reflectPoint(source);
        float newAtt = attenuation * (1.0f - walls[wi].energyAbsorption);

        if (newAtt < 1e-6f) continue;

        auto newPath = path;
        newPath.push_back(wi);

        ImageSource is;
        is.position    = reflected;
        is.attenuation = newAtt;
        is.order       = order;
        is.wallPath    = newPath;
        results.push_back(is);

        generateImageSources(reflected, walls, order + 1, maxOrder,
                             newAtt, newPath, results);
    }
}

bool ImageSourceMethod::isVisible(const Vec3f& from, const Vec3f& to,
                                   const std::vector<Wall>& walls) const {
    Vec3f dir = to - from;
    float maxDist = dir.norm();
    if (maxDist < 1e-8f) return true;
    dir /= maxDist;

    for (auto& wall : walls) {
        auto t = RayPicking::rayTriangleIntersect(from, dir, wall.triangle);
        if (t && *t > 1e-4f && *t < maxDist - 1e-4f) {
            return false;
        }
    }
    return true;
}

// ── Surface-level ISM with BVH visibility ───────────────────────────────────

std::vector<ImageSource> ImageSourceMethod::compute(
    const Vec3f& sourcePos,
    const Vec3f& listenerPos,
    const std::vector<AcousticSurface>& surfaces,
    const Bvh& bvh,
    int maxOrder)
{
    std::vector<ImageSource> results;

    if (isVisible(sourcePos, listenerPos, bvh)) {
        float dist = (listenerPos - sourcePos).norm();
        ImageSource direct;
        direct.position    = sourcePos;
        direct.attenuation = 1.0f / std::max(dist, 0.001f);
        direct.delay       = dist / SPEED_OF_SOUND;
        direct.order       = 0;
        results.push_back(direct);
    }

    generateImageSources(sourcePos, surfaces, 1, maxOrder, 1.0f, {}, results);

    std::vector<ImageSource> valid;
    for (auto& is : results) {
        if (is.order == 0) {
            valid.push_back(is);
            continue;
        }
        if (isVisible(is.position, listenerPos, bvh)) {
            float dist = (listenerPos - is.position).norm();
            is.attenuation /= std::max(dist, 0.001f);
            is.delay = dist / SPEED_OF_SOUND;
            valid.push_back(is);
        }
    }

    return valid;
}

void ImageSourceMethod::generateImageSources(
    const Vec3f& source,
    const std::vector<AcousticSurface>& surfaces,
    int order, int maxOrder,
    float attenuation,
    const std::vector<int>& path,
    std::vector<ImageSource>& results)
{
    if (order > maxOrder) return;

    for (int si = 0; si < static_cast<int>(surfaces.size()); ++si) {
        if (!path.empty() && path.back() == si) continue;

        Vec3f reflected = surfaces[si].reflectPoint(source);
        float newAtt = attenuation * (1.0f - surfaces[si].energyAbsorption);

        if (newAtt < 1e-6f) continue;

        auto newPath = path;
        newPath.push_back(si);

        ImageSource is;
        is.position    = reflected;
        is.attenuation = newAtt;
        is.order       = order;
        is.wallPath    = newPath;
        results.push_back(is);

        generateImageSources(reflected, surfaces, order + 1, maxOrder,
                             newAtt, newPath, results);
    }
}

bool ImageSourceMethod::isVisible(const Vec3f& from, const Vec3f& to,
                                   const Bvh& bvh) const {
    Vec3f dir = to - from;
    float maxDist = dir.norm();
    if (maxDist < 1e-8f) return true;
    dir /= maxDist;

    return !bvh.anyHit(from, dir, 1e-4f, maxDist - 1e-4f);
}

} // namespace prs
