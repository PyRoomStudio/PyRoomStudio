#include "GpuRayTracer.h"

#include <QDebug>

namespace prs {

GpuRayTracer::GpuRayTracer() = default;
GpuRayTracer::~GpuRayTracer() = default;

void GpuRayTracer::buildWallCache(const std::vector<Wall>& walls) {
    wallsCache_.clear();
    wallsCache_.reserve(walls.size());

    for (const auto& w : walls) {
        GpuWall gw;
        gw.v0 = w.triangle.v0;
        gw.v1 = w.triangle.v1;
        gw.v2 = w.triangle.v2;
        gw.normal = w.triangle.normal;
        gw.energyAbsorption = w.energyAbsorption;
        gw.scattering = w.scattering;
        wallsCache_.push_back(gw);
    }

    // A future GPU implementation would upload wallsCache_ to GPU buffers here.
}

std::vector<RayContribution> GpuRayTracer::trace(
    const Vec3f& sourcePos,
    const Vec3f& listenerPos,
    const std::vector<Wall>& walls,
    int numRays,
    float listenerRadius,
    int maxBounces,
    float minEnergy,
    const Vec3f* headCenter,
    float headRadius)
{
    buildWallCache(walls);

    // For now, fall back to the existing CPU implementation while keeping the
    // GPU-oriented data layout ready for a future kernel-based implementation.
    qWarning() << "GpuRayTracer::trace using CPU fallback; GPU kernels not yet implemented.";

    RayTracer cpuTracer;
    return cpuTracer.trace(
        sourcePos,
        listenerPos,
        walls,
        numRays,
        listenerRadius,
        maxBounces,
        minEnergy,
        headCenter,
        headRadius);
}

} // namespace prs

