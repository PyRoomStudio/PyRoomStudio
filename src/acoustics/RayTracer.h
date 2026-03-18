#pragma once

#include "core/Types.h"
#include "core/Material.h"
#include "Wall.h"
#include "Bvh.h"

#include <vector>
#include <array>

namespace prs {

struct RayContribution {
    float delay  = 0.0f;
    std::array<float, NUM_FREQ_BANDS> energy = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

class RayTracer {
public:
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
