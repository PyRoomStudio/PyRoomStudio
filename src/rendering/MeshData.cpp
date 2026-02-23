#include "MeshData.h"

#include <QFile>
#include <QDataStream>
#include <cstdint>
#include <limits>

namespace prs {

bool MeshData::loadSTL(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 84) return false;

    const char* ptr = data.constData();

    // Skip 80-byte header
    ptr += 80;

    uint32_t numTriangles = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += 4;

    size_t expectedSize = 84 + numTriangles * 50;
    if (static_cast<size_t>(data.size()) < expectedSize) return false;

    triangles_.clear();
    triangles_.reserve(numTriangles);

    for (uint32_t i = 0; i < numTriangles; ++i) {
        const float* fp = reinterpret_cast<const float*>(ptr);

        Triangle tri;
        tri.normal = Vec3f(fp[0], fp[1], fp[2]);
        tri.v0     = Vec3f(fp[3], fp[4], fp[5]);
        tri.v1     = Vec3f(fp[6], fp[7], fp[8]);
        tri.v2     = Vec3f(fp[9], fp[10], fp[11]);

        triangles_.push_back(tri);

        // 12 floats (48 bytes) + 2-byte attribute
        ptr += 50;
    }

    filePath_ = filepath;
    computeBounds();
    return true;
}

void MeshData::computeBounds() {
    if (triangles_.empty()) {
        center_ = Vec3f::Zero();
        min_ = max_ = Vec3f::Zero();
        size_ = 0.0f;
        return;
    }

    min_ = Vec3f(std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max());
    max_ = Vec3f(std::numeric_limits<float>::lowest(),
                 std::numeric_limits<float>::lowest(),
                 std::numeric_limits<float>::lowest());

    for (auto& tri : triangles_) {
        for (const Vec3f* v : {&tri.v0, &tri.v1, &tri.v2}) {
            min_ = min_.cwiseMin(*v);
            max_ = max_.cwiseMax(*v);
        }
    }

    center_ = (min_ + max_) * 0.5f;
    size_   = (max_ - min_).norm();
}

std::vector<Vec3f> MeshData::flatVertices() const {
    std::vector<Vec3f> verts;
    verts.reserve(triangles_.size() * 3);
    for (auto& tri : triangles_) {
        verts.push_back(tri.v0);
        verts.push_back(tri.v1);
        verts.push_back(tri.v2);
    }
    return verts;
}

std::vector<Vec3f> MeshData::scaledFlatVertices(float scaleFactor) const {
    auto verts = flatVertices();
    for (auto& v : verts) v *= scaleFactor;
    return verts;
}

} // namespace prs
