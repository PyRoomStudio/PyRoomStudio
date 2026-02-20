#include <cmath>
#include <random>
#include <algorithm>

#include "RayTracer.h"
#include "rendering/RayPicking.h"

namespace prs {

std::vector<RayContribution> RayTracer::trace(
    const Vec3f& sourcePos,
    const Vec3f& listenerPos,
    const std::vector<Wall>& walls,
    int numRays,
    float listenerRadius,
    int maxBounces,
    float minEnergy)
{
    std::vector<RayContribution> contributions;

    for (int r = 0; r < numRays; ++r) {
        Vec3f rayOrigin = sourcePos;
        Vec3f rayDir    = randomDirectionOnSphere();
        float energy    = 1.0f;
        float totalDist = 0.0f;

        for (int bounce = 0; bounce < maxBounces && energy > minEnergy; ++bounce) {
            // Check if ray passes near listener (detection sphere)
            auto tListener = RayPicking::raySphereIntersect(
                rayOrigin, rayDir, listenerPos, listenerRadius);
            if (tListener) {
                float detDist = totalDist + *tListener;
                RayContribution rc;
                rc.delay  = detDist / SPEED_OF_SOUND;
                rc.energy = energy / (4.0f * static_cast<float>(M_PI) * detDist * detDist + 1e-8f);
                contributions.push_back(rc);
            }

            // Find nearest wall intersection
            float wallT = 0.0f;
            int wallIdx = findNearestWall(rayOrigin, rayDir, walls, wallT);
            if (wallIdx < 0) break;

            totalDist += wallT;

            // Move to wall hit point
            Vec3f hitPoint = rayOrigin + rayDir * wallT;

            // Apply wall absorption
            energy *= (1.0f - walls[wallIdx].energyAbsorption);

            // Compute reflected direction (specular + diffuse)
            rayDir = reflectDirection(rayDir, walls[wallIdx].normal(), walls[wallIdx].scattering);
            rayOrigin = hitPoint + rayDir * 1e-4f;

            // Air absorption (simple frequency-independent model)
            float airAbsCoeff = 0.001f;
            energy *= std::exp(-airAbsCoeff * wallT);
        }
    }

    return contributions;
}

Vec3f RayTracer::randomDirectionOnSphere() const {
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float theta = 2.0f * static_cast<float>(M_PI) * dist(rng);
    float phi   = std::acos(1.0f - 2.0f * dist(rng));

    return Vec3f(
        std::sin(phi) * std::cos(theta),
        std::sin(phi) * std::sin(theta),
        std::cos(phi)
    );
}

int RayTracer::findNearestWall(const Vec3f& origin, const Vec3f& dir,
                                const std::vector<Wall>& walls, float& outT) const {
    float minT = std::numeric_limits<float>::max();
    int hitIdx = -1;

    for (int i = 0; i < static_cast<int>(walls.size()); ++i) {
        auto t = RayPicking::rayTriangleIntersect(origin, dir, walls[i].triangle);
        if (t && *t > 1e-4f && *t < minT) {
            minT = *t;
            hitIdx = i;
        }
    }

    outT = minT;
    return hitIdx;
}

Vec3f RayTracer::reflectDirection(const Vec3f& dir, const Vec3f& normal, float scattering) const {
    // Specular reflection
    Vec3f specular = dir - 2.0f * dir.dot(normal) * normal;

    if (scattering < 1e-6f) return specular.normalized();

    // Diffuse direction: random hemisphere around normal
    Vec3f diffuse = randomDirectionOnSphere();
    if (diffuse.dot(normal) < 0) diffuse = -diffuse;

    // Blend specular and diffuse based on scattering coefficient
    Vec3f blended = (1.0f - scattering) * specular + scattering * diffuse;
    return blended.normalized();
}

} // namespace prs
