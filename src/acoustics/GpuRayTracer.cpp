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
        gw.absorption = w.absorption;
        gw.scattering = w.scattering;
        wallsCache_.push_back(gw);
    }
}

std::vector<RayContribution> GpuRayTracer::trace(
    const Vec3f& sourcePos,
    const Vec3f& listenerPos,
    const std::vector<Wall>& walls,
    const Bvh& bvh,
    int numRays,
    float listenerRadius,
    int maxBounces,
    float minEnergy,
    const Vec3f* headCenter,
    float headRadius)
{
    buildWallCache(walls);

    qWarning() << "GpuRayTracer::trace using CPU fallback; GPU kernels not yet implemented.";

    RayTracer cpuTracer;
    return cpuTracer.trace(
        sourcePos,
        listenerPos,
        walls,
        bvh,
        numRays,
        listenerRadius,
        maxBounces,
        minEnergy,
        headCenter,
        headRadius);
}

} // namespace prs
