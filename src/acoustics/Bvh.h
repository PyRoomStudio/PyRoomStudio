#pragma once

#include "core/Types.h"
#include "Wall.h"

#include <limits>
#include <vector>

namespace prs {

struct AABB {
    Vec3f min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    Vec3f max{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
              std::numeric_limits<float>::lowest()};

    void expand(const Vec3f& p) {
        min = min.cwiseMin(p);
        max = max.cwiseMax(p);
    }

    void expand(const AABB& other) {
        min = min.cwiseMin(other.min);
        max = max.cwiseMax(other.max);
    }

    Vec3f extent() const { return max - min; }
    float surfaceArea() const {
        Vec3f d = extent();
        return 2.0f * (d.x() * d.y() + d.y() * d.z() + d.z() * d.x());
    }

    float halfArea() const {
        Vec3f d = extent();
        return d.x() * d.y() + d.y() * d.z() + d.z() * d.x();
    }

    int longestAxis() const {
        Vec3f d = extent();
        if (d.x() >= d.y() && d.x() >= d.z())
            return 0;
        if (d.y() >= d.z())
            return 1;
        return 2;
    }
};

struct BvhNode {
    AABB bounds;
    int leftChild = -1;
    int rightChild = -1;
    int firstPrim = -1;
    int primCount = 0;

    bool isLeaf() const { return primCount > 0; }
};

struct BvhHit {
    int wallIndex = -1;
    float t = std::numeric_limits<float>::max();
};

class Bvh {
  public:
    void build(const std::vector<Wall>& walls);

    BvhHit closestHit(const Vec3f& origin, const Vec3f& dir, float tMin = 1e-4f) const;

    bool anyHit(const Vec3f& origin, const Vec3f& dir, float tMin, float tMax) const;

    bool empty() const { return nodes_.empty(); }

  private:
    static constexpr int MAX_LEAF_SIZE = 4;
    static constexpr int SAH_BINS = 12;
    static constexpr float TRAVERSAL_COST = 1.0f;
    static constexpr float INTERSECT_COST = 1.0f;

    void buildRecursive(int nodeIdx, int start, int end);
    static AABB triangleBounds(const Triangle& tri);
    bool intersectAABB(const Vec3f& origin, const Vec3f& invDir, const AABB& box, float tMin, float tMax) const;

    std::vector<BvhNode> nodes_;
    std::vector<int> primIndices_;
    std::vector<AABB> primBounds_;
    std::vector<Vec3f> primCentroids_;
    const std::vector<Wall>* walls_ = nullptr;
};

} // namespace prs
