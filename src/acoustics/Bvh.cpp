#include "Bvh.h"

#include "rendering/RayPicking.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace prs {

AABB Bvh::triangleBounds(const Triangle& tri) {
    AABB box;
    box.expand(tri.v0);
    box.expand(tri.v1);
    box.expand(tri.v2);
    return box;
}

void Bvh::build(const std::vector<Wall>& walls) {
    walls_ = &walls;
    int n = static_cast<int>(walls.size());
    if (n == 0)
        return;

    primIndices_.resize(n);
    primBounds_.resize(n);
    primCentroids_.resize(n);

    for (int i = 0; i < n; ++i) {
        primIndices_[i] = i;
        primBounds_[i] = triangleBounds(walls[i].triangle);
        primCentroids_[i] = (primBounds_[i].min + primBounds_[i].max) * 0.5f;
    }

    nodes_.reserve(2 * n);
    nodes_.push_back(BvhNode{});
    buildRecursive(0, 0, n);
}

void Bvh::buildRecursive(int nodeIdx, int start, int end) {
    BvhNode& node = nodes_[nodeIdx];
    int count = end - start;

    AABB nodeBounds;
    for (int i = start; i < end; ++i)
        nodeBounds.expand(primBounds_[primIndices_[i]]);
    node.bounds = nodeBounds;

    if (count <= MAX_LEAF_SIZE) {
        node.firstPrim = start;
        node.primCount = count;
        return;
    }

    AABB centroidBounds;
    for (int i = start; i < end; ++i)
        centroidBounds.expand(primCentroids_[primIndices_[i]]);

    int axis = centroidBounds.longestAxis();
    float axisExtent = centroidBounds.max[axis] - centroidBounds.min[axis];

    if (axisExtent < 1e-8f) {
        node.firstPrim = start;
        node.primCount = count;
        return;
    }

    // SAH binned partitioning
    struct Bin {
        AABB bounds;
        int count = 0;
    };
    Bin bins[SAH_BINS];

    float scale = static_cast<float>(SAH_BINS) / axisExtent;
    for (int i = start; i < end; ++i) {
        int idx = primIndices_[i];
        int b =
            std::min(static_cast<int>((primCentroids_[idx][axis] - centroidBounds.min[axis]) * scale), SAH_BINS - 1);
        bins[b].count++;
        bins[b].bounds.expand(primBounds_[idx]);
    }

    float bestCost = std::numeric_limits<float>::max();
    int bestSplit = -1;

    // Sweep from left and right to compute SAH cost for each split plane
    float leftArea[SAH_BINS - 1];
    float rightArea[SAH_BINS - 1];
    int leftCount[SAH_BINS - 1];
    int rightCount[SAH_BINS - 1];

    AABB leftBox;
    int leftN = 0;
    for (int i = 0; i < SAH_BINS - 1; ++i) {
        leftBox.expand(bins[i].bounds);
        leftN += bins[i].count;
        leftArea[i] = leftBox.halfArea();
        leftCount[i] = leftN;
    }

    AABB rightBox;
    int rightN = 0;
    for (int i = SAH_BINS - 1; i > 0; --i) {
        rightBox.expand(bins[i].bounds);
        rightN += bins[i].count;
        rightArea[i - 1] = rightBox.halfArea();
        rightCount[i - 1] = rightN;
    }

    float nodeArea = nodeBounds.halfArea();
    for (int i = 0; i < SAH_BINS - 1; ++i) {
        if (leftCount[i] == 0 || rightCount[i] == 0)
            continue;
        float cost =
            TRAVERSAL_COST + (leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i]) * INTERSECT_COST / nodeArea;
        if (cost < bestCost) {
            bestCost = cost;
            bestSplit = i;
        }
    }

    float leafCost = static_cast<float>(count) * INTERSECT_COST;
    if (bestSplit < 0 || bestCost >= leafCost) {
        node.firstPrim = start;
        node.primCount = count;
        return;
    }

    float splitPos = centroidBounds.min[axis] + (bestSplit + 1) * axisExtent / SAH_BINS;

    auto mid = std::partition(primIndices_.begin() + start, primIndices_.begin() + end,
                              [&](int idx) { return primCentroids_[idx][axis] < splitPos; });

    int midIdx = static_cast<int>(mid - primIndices_.begin());
    if (midIdx == start || midIdx == end) {
        midIdx = start + count / 2;
        std::nth_element(primIndices_.begin() + start, primIndices_.begin() + midIdx, primIndices_.begin() + end,
                         [&](int a, int b) { return primCentroids_[a][axis] < primCentroids_[b][axis]; });
    }

    int leftIdx = static_cast<int>(nodes_.size());
    nodes_.push_back(BvhNode{});
    int rightIdx = static_cast<int>(nodes_.size());
    nodes_.push_back(BvhNode{});

    nodes_[nodeIdx].leftChild = leftIdx;
    nodes_[nodeIdx].rightChild = rightIdx;

    buildRecursive(leftIdx, start, midIdx);
    buildRecursive(rightIdx, midIdx, end);
}

bool Bvh::intersectAABB(const Vec3f& origin, const Vec3f& invDir, const AABB& box, float tMin, float tMax) const {
    for (int i = 0; i < 3; ++i) {
        float t1 = (box.min[i] - origin[i]) * invDir[i];
        float t2 = (box.max[i] - origin[i]) * invDir[i];
        if (t1 > t2)
            std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax)
            return false;
    }
    return true;
}

BvhHit Bvh::closestHit(const Vec3f& origin, const Vec3f& dir, float tMin) const {
    BvhHit result;
    if (nodes_.empty())
        return result;

    Vec3f invDir(1.0f / dir.x(), 1.0f / dir.y(), 1.0f / dir.z());

    // Stack-based traversal
    int stack[64];
    int stackPtr = 0;
    stack[stackPtr++] = 0;

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        const BvhNode& node = nodes_[nodeIdx];

        if (!intersectAABB(origin, invDir, node.bounds, tMin, result.t))
            continue;

        if (node.isLeaf()) {
            for (int i = 0; i < node.primCount; ++i) {
                int wallIdx = primIndices_[node.firstPrim + i];
                auto t = RayPicking::rayTriangleIntersect(origin, dir, (*walls_)[wallIdx].triangle);
                if (t && *t > tMin && *t < result.t) {
                    result.t = *t;
                    result.wallIndex = wallIdx;
                }
            }
        } else {
            stack[stackPtr++] = node.leftChild;
            stack[stackPtr++] = node.rightChild;
        }
    }

    return result;
}

bool Bvh::anyHit(const Vec3f& origin, const Vec3f& dir, float tMin, float tMax) const {
    if (nodes_.empty())
        return false;

    Vec3f invDir(1.0f / dir.x(), 1.0f / dir.y(), 1.0f / dir.z());

    int stack[64];
    int stackPtr = 0;
    stack[stackPtr++] = 0;

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        const BvhNode& node = nodes_[nodeIdx];

        if (!intersectAABB(origin, invDir, node.bounds, tMin, tMax))
            continue;

        if (node.isLeaf()) {
            for (int i = 0; i < node.primCount; ++i) {
                int wallIdx = primIndices_[node.firstPrim + i];
                auto t = RayPicking::rayTriangleIntersect(origin, dir, (*walls_)[wallIdx].triangle);
                if (t && *t > tMin && *t < tMax)
                    return true;
            }
        } else {
            stack[stackPtr++] = node.leftChild;
            stack[stackPtr++] = node.rightChild;
        }
    }

    return false;
}

} // namespace prs
