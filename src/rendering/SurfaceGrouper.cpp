#include "SurfaceGrouper.h"

#include <queue>

#include <cmath>

namespace prs {

SurfaceGrouper::EdgeSet SurfaceGrouper::computeFeatureEdges(const MeshData& mesh, float angleThresholdDeg) {
    const auto& tris = mesh.triangles();
    std::map<Edge, std::vector<int>> edgeToTris;

    for (int i = 0; i < static_cast<int>(tris.size()); ++i) {
        const Vec3f* verts[3] = {&tris[i].v0, &tris[i].v1, &tris[i].v2};
        for (int j = 0; j < 3; ++j) {
            Edge e = makeEdge(*verts[j], *verts[(j + 1) % 3]);
            edgeToTris[e].push_back(i);
        }
    }

    EdgeSet featureEdges;
    float thresholdRad = angleThresholdDeg * static_cast<float>(M_PI) / 180.0f;

    for (auto& [edge, triIndices] : edgeToTris) {
        if (triIndices.size() == 1) {
            featureEdges.insert(edge);
        } else if (triIndices.size() == 2) {
            Vec3f n1 = tris[triIndices[0]].normal.normalized();
            Vec3f n2 = tris[triIndices[1]].normal.normalized();
            float dotVal = std::clamp(n1.dot(n2), -1.0f, 1.0f);
            float angle = std::acos(dotVal);
            if (angle > thresholdRad) {
                featureEdges.insert(edge);
            }
        }
    }

    return featureEdges;
}

std::vector<std::set<int>> SurfaceGrouper::groupTrianglesIntoSurfaces(const MeshData& mesh,
                                                                      const EdgeSet& featureEdges) {
    const auto& tris = mesh.triangles();
    int n = static_cast<int>(tris.size());

    std::map<Edge, std::vector<int>> edgeToTris;
    for (int i = 0; i < n; ++i) {
        const Vec3f* verts[3] = {&tris[i].v0, &tris[i].v1, &tris[i].v2};
        for (int j = 0; j < 3; ++j) {
            Edge e = makeEdge(*verts[j], *verts[(j + 1) % 3]);
            edgeToTris[e].push_back(i);
        }
    }

    std::vector<bool> visited(n, false);
    std::vector<std::set<int>> surfaces;

    for (int i = 0; i < n; ++i) {
        if (visited[i])
            continue;

        std::set<int> surface;
        std::queue<int> queue;
        queue.push(i);

        while (!queue.empty()) {
            int t = queue.front();
            queue.pop();
            if (visited[t])
                continue;
            visited[t] = true;
            surface.insert(t);

            const Vec3f* verts[3] = {&tris[t].v0, &tris[t].v1, &tris[t].v2};
            for (int j = 0; j < 3; ++j) {
                Edge e = makeEdge(*verts[j], *verts[(j + 1) % 3]);
                if (featureEdges.count(e))
                    continue;
                for (int neighbor : edgeToTris[e]) {
                    if (!visited[neighbor])
                        queue.push(neighbor);
                }
            }
        }
        surfaces.push_back(std::move(surface));
    }
    return surfaces;
}

} // namespace prs
