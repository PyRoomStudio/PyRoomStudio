#pragma once

#include "core/Types.h"
#include "core/Material.h"
#include "acoustics/Wall.h"

#include <vector>
#include <array>

namespace prs {

class MeshSimplifier {
public:
    // Simplify a wall list to targetCount triangles using QEM edge collapse.
    // surfaceIds maps each wall index to its surface group; collapses across
    // different surface groups are forbidden so material boundaries are preserved.
    static std::vector<Wall> simplify(const std::vector<Wall>& walls,
                                       const std::vector<int>& surfaceIds,
                                       int targetCount);

    struct Vertex {
        Vec3f pos;
        Eigen::Matrix4f quadric = Eigen::Matrix4f::Zero();
        int surfaceId = -1;
        bool boundary = false;
    };

private:
    struct Face {
        int v[3]{-1, -1, -1};
        int surfaceId = -1;
        std::array<float, NUM_FREQ_BANDS> absorption = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f};
        float scattering = 0.1f;
        bool removed = false;
    };

    struct EdgeCollapse {
        int v0, v1;
        Vec3f optimalPos;
        float cost = 0.0f;

        bool operator>(const EdgeCollapse& o) const { return cost > o.cost; }
    };

    static Eigen::Matrix4f computePlaneQuadric(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2);
};

} // namespace prs
