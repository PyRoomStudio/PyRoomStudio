#pragma once

#include "RayTracer.h"

#include <array>
#include <vector>

namespace prs {

class GpuRayTracer {
  public:
    GpuRayTracer();
    ~GpuRayTracer();

    std::vector<RayContribution> trace(const Vec3f& sourcePos, const Vec3f& listenerPos, const std::vector<Wall>& walls,
                                       const Bvh& bvh, int numRays, float listenerRadius = 0.5f, int maxBounces = 100,
                                       float minEnergy = 1e-6f, const Vec3f* headCenter = nullptr,
                                       float headRadius = 0.0f);

    struct GpuWall {
        Vec3f v0;
        Vec3f v1;
        Vec3f v2;
        Vec3f normal;
        std::array<float, NUM_FREQ_BANDS> absorption;
        float scattering;
    };

  private:
    void buildWallCache(const std::vector<Wall>& walls);

    std::vector<GpuWall> wallsCache_;
};

} // namespace prs
