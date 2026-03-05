#pragma once

#include "core/Types.h"
#include "Wall.h"
#include "Bvh.h"

#include <vector>

namespace prs {

struct RayContribution {
    float delay  = 0.0f;
    float energy = 0.0f;
};

class RayTracer {
public:
    /** headCenter/headRadius: when headRadius > 0, contributions that pass through the head sphere are skipped. */
    std::vector<RayContribution> trace(
        const Vec3f& sourcePos,
        const Vec3f& listenerPos,
        const std::vector<Wall>& walls,
        const Bvh& bvh,
        int numRays,
        float listenerRadius = 0.5f,
        int maxBounces = 100,
        float minEnergy = 1e-6f,
        const Vec3f* headCenter = nullptr,
        float headRadius = 0.0f,
        bool airAbsorption = true);

private:
    Vec3f randomDirectionOnSphere() const;
    Vec3f reflectDirection(const Vec3f& dir, const Vec3f& normal, float scattering) const;
};

} // namespace prs
