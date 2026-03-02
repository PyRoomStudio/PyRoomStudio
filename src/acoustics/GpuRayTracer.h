#pragma once

#include "RayTracer.h"

#include <vector>

namespace prs {

/**
 * GPU-oriented implementation of IRayTracer.
 *
 * The current implementation is a thin wrapper around the CPU RayTracer that
 * also prepares GPU-friendly wall data. This keeps the interface stable while
 * allowing a future implementation to replace the CPU fallback with actual
 * GPU kernels without touching the rest of the codebase.
 */
class GpuRayTracer : public IRayTracer {
public:
    GpuRayTracer();
    ~GpuRayTracer() override;

    std::vector<RayContribution> trace(
        const Vec3f& sourcePos,
        const Vec3f& listenerPos,
        const std::vector<Wall>& walls,
        int numRays,
        float listenerRadius = 0.5f,
        int maxBounces = 100,
        float minEnergy = 1e-6f,
        const Vec3f* headCenter = nullptr,
        float headRadius = 0.0f) override;

    struct GpuWall {
        Vec3f v0;
        Vec3f v1;
        Vec3f v2;
        Vec3f normal;
        float energyAbsorption;
        float scattering;
    };

private:
    void buildWallCache(const std::vector<Wall>& walls);

    std::vector<GpuWall> wallsCache_;
};

} // namespace prs

