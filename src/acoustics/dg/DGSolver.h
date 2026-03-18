#pragma once

#include "DGTypes.h"
#include "core/Types.h"
#include "rendering/Viewport3D.h"
#include "acoustics/Bvh.h"

#include <vector>
#include <functional>
#include <QString>

namespace prs {
namespace dg {

class DGSolver {
public:
    struct Result {
        std::vector<float> impulseResponse;
        float sampleRate = 0.0f;
        float duration = 0.0f;
    };

    // Solve for a single source-listener pair.
    // Returns an impulse response suitable for convolution.
    Result solve(
        const Vec3f& sourcePos,
        const Vec3f& listenerPos,
        const std::vector<Viewport3D::WallInfo>& walls,
        const std::vector<Vec3f>& modelVertices,
        const Bvh& bvh,
        const DGParams& params,
        std::function<void(int, const QString&)> progressCallback = nullptr,
        std::function<bool()> cancelCheck = nullptr);

private:
    Result solve2D(
        const Vec3f& sourcePos,
        const Vec3f& listenerPos,
        const std::vector<Viewport3D::WallInfo>& walls,
        const std::vector<Vec3f>& modelVertices,
        const DGParams& params,
        std::function<void(int, const QString&)> progressCallback,
        std::function<bool()> cancelCheck);

    Result solve3D(
        const Vec3f& sourcePos,
        const Vec3f& listenerPos,
        const std::vector<Viewport3D::WallInfo>& walls,
        const std::vector<Vec3f>& modelVertices,
        const Bvh& bvh,
        const DGParams& params,
        std::function<void(int, const QString&)> progressCallback,
        std::function<bool()> cancelCheck);
};

} // namespace dg
} // namespace prs
