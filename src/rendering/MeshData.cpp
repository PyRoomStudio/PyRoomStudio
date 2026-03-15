#include "MeshData.h"

#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QTextStream>
#include <cstdint>
#include <limits>
#include <map>
#include <sstream>

namespace prs {

bool MeshData::load(const QString& filepath) {
    QString ext = QFileInfo(filepath).suffix().toLower();
    if (ext == "obj")
        return loadOBJ(filepath);
    return loadSTL(filepath);
}

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

bool MeshData::loadOBJ(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    std::vector<Vec3f> vertices;
    triangles_.clear();

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line[0] == '#')
            continue;

        std::string sline = line.toStdString();
        std::istringstream iss(sline);
        std::string token;
        iss >> token;

        if (token == "v") {
            float x, y, z;
            if (iss >> x >> y >> z)
                vertices.emplace_back(x, y, z);
        } else if (token == "f") {
            std::vector<int> faceIndices;
            std::string vert;
            while (iss >> vert) {
                int idx = 0;
                std::istringstream vis(vert);
                vis >> idx;
                if (idx > 0)
                    faceIndices.push_back(idx - 1);
                else if (idx < 0)
                    faceIndices.push_back(static_cast<int>(vertices.size()) + idx);
            }

            // Triangulate convex polygon (fan from first vertex)
            for (size_t i = 2; i < faceIndices.size(); ++i) {
                int i0 = faceIndices[0];
                int i1 = faceIndices[i - 1];
                int i2 = faceIndices[i];
                if (i0 < 0 || i0 >= static_cast<int>(vertices.size()) ||
                    i1 < 0 || i1 >= static_cast<int>(vertices.size()) ||
                    i2 < 0 || i2 >= static_cast<int>(vertices.size()))
                    continue;

                Triangle tri;
                tri.v0 = vertices[i0];
                tri.v1 = vertices[i1];
                tri.v2 = vertices[i2];
                Vec3f e1 = tri.v1 - tri.v0;
                Vec3f e2 = tri.v2 - tri.v0;
                tri.normal = e1.cross(e2);
                float len = tri.normal.norm();
                if (len > 1e-8f)
                    tri.normal /= len;
                triangles_.push_back(tri);
            }
        }
    }

    if (triangles_.empty())
        return false;

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

int MeshData::boundaryEdgeCount() const {
    if (triangles_.empty()) return 0;

    std::map<Edge, int> edgeFaceCount;
    for (const auto& tri : triangles_) {
        const Vec3f* verts[3] = {&tri.v0, &tri.v1, &tri.v2};
        for (int j = 0; j < 3; ++j) {
            Edge e = makeEdge(*verts[j], *verts[(j + 1) % 3]);
            edgeFaceCount[e]++;
        }
    }

    int boundary = 0;
    for (const auto& [edge, count] : edgeFaceCount) {
        if (count != 2) ++boundary;
    }
    return boundary;
}

bool MeshData::isClosed() const {
    return !triangles_.empty() && boundaryEdgeCount() == 0;
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
