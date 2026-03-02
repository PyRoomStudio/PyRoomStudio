#pragma once

#include "core/Types.h"
#include "Wall.h"
#include "Bvh.h"

#include <vector>

namespace prs {

struct ImageSource {
    Vec3f position;
    float attenuation = 1.0f;
    float delay       = 0.0f;
    int   order       = 0;
    std::vector<int> wallPath;
};

class ImageSourceMethod {
public:
    // Original interface (linear scan, per-triangle walls)
    std::vector<ImageSource> compute(
        const Vec3f& sourcePos,
        const Vec3f& listenerPos,
        const std::vector<Wall>& walls,
        int maxOrder);

    // Surface-level ISM with BVH-accelerated visibility
    std::vector<ImageSource> compute(
        const Vec3f& sourcePos,
        const Vec3f& listenerPos,
        const std::vector<AcousticSurface>& surfaces,
        const Bvh& bvh,
        int maxOrder);

private:
    void generateImageSources(
        const Vec3f& source,
        const std::vector<Wall>& walls,
        int order, int maxOrder,
        float attenuation,
        const std::vector<int>& path,
        std::vector<ImageSource>& results);

    void generateImageSources(
        const Vec3f& source,
        const std::vector<AcousticSurface>& surfaces,
        int order, int maxOrder,
        float attenuation,
        const std::vector<int>& path,
        std::vector<ImageSource>& results);

    bool isVisible(const Vec3f& from, const Vec3f& to,
                   const std::vector<Wall>& walls) const;

    bool isVisible(const Vec3f& from, const Vec3f& to,
                   const Bvh& bvh) const;
};

} // namespace prs
