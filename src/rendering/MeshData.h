#pragma once

#include "core/Types.h"
#include <QString>
#include <vector>

namespace prs {

class MeshData {
public:
    MeshData() = default;

    bool load(const QString& filepath);
    bool loadSTL(const QString& filepath);
    bool loadOBJ(const QString& filepath);

    const std::vector<Triangle>& triangles() const { return triangles_; }
    int triangleCount() const { return static_cast<int>(triangles_.size()); }
    QString filePath() const { return filePath_; }

    Vec3f center() const { return center_; }
    float diagonalSize() const { return size_; }

    Vec3f minBound() const { return min_; }
    Vec3f maxBound() const { return max_; }

    bool isClosed() const;
    int  boundaryEdgeCount() const;

    std::vector<Vec3f> flatVertices() const;
    std::vector<Vec3f> scaledFlatVertices(float scaleFactor) const;

private:
    void computeBounds();

    std::vector<Triangle> triangles_;
    Vec3f center_ = Vec3f::Zero();
    Vec3f min_    = Vec3f::Zero();
    Vec3f max_    = Vec3f::Zero();
    float size_   = 0.0f;
    QString filePath_;
};

} // namespace prs
