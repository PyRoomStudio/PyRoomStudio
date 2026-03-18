#include <cmath>
#include <random>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "RayTracer.h"
#include "rendering/RayPicking.h"

namespace prs {

static bool allBelowThreshold(const std::array<float, NUM_FREQ_BANDS>& energy, float threshold) {
    for (float e : energy)
        if (e > threshold) return false;
    return true;
}

std::vector<RayContribution> RayTracer::trace(
    const Vec3f& sourcePos,
    const Vec3f& listenerPos,
    const std::vector<Wall>& walls,
    const Bvh& bvh,
    int numRays,
    float listenerRadius,
    int maxBounces,
    float minEnergy,
    const Vec3f* headCenter,
    float headRadius,
    bool airAbsorption)
{
    std::vector<RayContribution> contributions;
    bool useBvh = !bvh.empty();
    const float airAbsCoeff = airAbsorption ? 0.005f : 0.0f;

    std::array<float, NUM_FREQ_BANDS> initEnergy;
    initEnergy.fill(1.0f);

#ifdef _OPENMP
    #pragma omp parallel
    {
        std::vector<RayContribution> localContribs;

        #pragma omp for schedule(dynamic, 64)
        for (int r = 0; r < numRays; ++r) {
            Vec3f rayOrigin = sourcePos;
            Vec3f rayDir    = randomDirectionOnSphere();
            auto energy     = initEnergy;
            float totalDist = 0.0f;

            for (int bounce = 0; bounce < maxBounces && !allBelowThreshold(energy, minEnergy); ++bounce) {
                auto tListener = RayPicking::raySphereIntersect(
                    rayOrigin, rayDir, listenerPos, listenerRadius);
                if (tListener) {
                    Vec3f hitPoint = rayOrigin + rayDir * *tListener;
                    bool throughHead = (headRadius > 0.0f && headCenter != nullptr &&
                        RayPicking::segmentPassesThroughSphere(rayOrigin, hitPoint, *headCenter, headRadius));
                    if (!throughHead) {
                        float detDist = totalDist + *tListener;
                        float geomAtten = 1.0f / (4.0f * static_cast<float>(M_PI) * detDist * detDist + 1e-8f);
                        RayContribution rc;
                        rc.delay = detDist / SPEED_OF_SOUND;
                        for (int b = 0; b < NUM_FREQ_BANDS; ++b)
                            rc.energy[b] = energy[b] * geomAtten;
                        localContribs.push_back(rc);
                    }
                }

                int wallIdx = -1;
                float wallT = 0.0f;

                if (useBvh) {
                    auto hit = bvh.closestHit(rayOrigin, rayDir);
                    wallIdx = hit.wallIndex;
                    wallT = hit.t;
                } else {
                    float minT = std::numeric_limits<float>::max();
                    for (int i = 0; i < static_cast<int>(walls.size()); ++i) {
                        auto t = RayPicking::rayTriangleIntersect(rayOrigin, rayDir, walls[i].triangle);
                        if (t && *t > 1e-4f && *t < minT) {
                            minT = *t;
                            wallIdx = i;
                        }
                    }
                    wallT = minT;
                }

                if (wallIdx < 0) break;

                totalDist += wallT;
                Vec3f hitPoint = rayOrigin + rayDir * wallT;
                for (int b = 0; b < NUM_FREQ_BANDS; ++b)
                    energy[b] *= (1.0f - walls[wallIdx].absorption[b]);
                rayDir = reflectDirection(rayDir, walls[wallIdx].normal(), walls[wallIdx].scattering);
                rayOrigin = hitPoint + rayDir * 1e-4f;

                if (airAbsCoeff > 0.0f) {
                    float airFactor = std::exp(-airAbsCoeff * wallT);
                    for (int b = 0; b < NUM_FREQ_BANDS; ++b)
                        energy[b] *= airFactor;
                }
            }
        }

        #pragma omp critical
        contributions.insert(contributions.end(), localContribs.begin(), localContribs.end());
    }
#else
    for (int r = 0; r < numRays; ++r) {
        Vec3f rayOrigin = sourcePos;
        Vec3f rayDir    = randomDirectionOnSphere();
        auto energy     = initEnergy;
        float totalDist = 0.0f;

        for (int bounce = 0; bounce < maxBounces && !allBelowThreshold(energy, minEnergy); ++bounce) {
            auto tListener = RayPicking::raySphereIntersect(
                rayOrigin, rayDir, listenerPos, listenerRadius);
            if (tListener) {
                Vec3f hitPoint = rayOrigin + rayDir * *tListener;
                bool throughHead = (headRadius > 0.0f && headCenter != nullptr &&
                    RayPicking::segmentPassesThroughSphere(rayOrigin, hitPoint, *headCenter, headRadius));
                if (!throughHead) {
                    float detDist = totalDist + *tListener;
                    float geomAtten = 1.0f / (4.0f * static_cast<float>(M_PI) * detDist * detDist + 1e-8f);
                    RayContribution rc;
                    rc.delay = detDist / SPEED_OF_SOUND;
                    for (int b = 0; b < NUM_FREQ_BANDS; ++b)
                        rc.energy[b] = energy[b] * geomAtten;
                    contributions.push_back(rc);
                }
            }

            int wallIdx = -1;
            float wallT = 0.0f;

            if (useBvh) {
                auto hit = bvh.closestHit(rayOrigin, rayDir);
                wallIdx = hit.wallIndex;
                wallT = hit.t;
            } else {
                float minT = std::numeric_limits<float>::max();
                for (int i = 0; i < static_cast<int>(walls.size()); ++i) {
                    auto t = RayPicking::rayTriangleIntersect(rayOrigin, rayDir, walls[i].triangle);
                    if (t && *t > 1e-4f && *t < minT) {
                        minT = *t;
                        wallIdx = i;
                    }
                }
                wallT = minT;
            }

            if (wallIdx < 0) break;

            totalDist += wallT;
            Vec3f hitPoint = rayOrigin + rayDir * wallT;
            for (int b = 0; b < NUM_FREQ_BANDS; ++b)
                energy[b] *= (1.0f - walls[wallIdx].absorption[b]);
            rayDir = reflectDirection(rayDir, walls[wallIdx].normal(), walls[wallIdx].scattering);
            rayOrigin = hitPoint + rayDir * 1e-4f;

            if (airAbsCoeff > 0.0f) {
                float airFactor = std::exp(-airAbsCoeff * wallT);
                for (int b = 0; b < NUM_FREQ_BANDS; ++b)
                    energy[b] *= airFactor;
            }
        }
    }
#endif

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

Vec3f RayTracer::reflectDirection(const Vec3f& dir, const Vec3f& normal, float scattering) const {
    Vec3f specular = dir - 2.0f * dir.dot(normal) * normal;

    if (scattering < 1e-6f) return specular.normalized();

    Vec3f diffuse = randomDirectionOnSphere();
    if (diffuse.dot(normal) < 0) diffuse = -diffuse;

    Vec3f blended = (1.0f - scattering) * specular + scattering * diffuse;
    return blended.normalized();
}

} // namespace prs
