#include "MeshSimplifier.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <tuple>

namespace prs {

namespace {

struct PairHash {
    std::size_t operator()(const std::pair<int,int>& p) const {
        auto h1 = std::hash<int>{}(p.first);
        auto h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 * 2654435761u);
    }
};

using EdgeKey = std::pair<int,int>;

EdgeKey makeEdgeKey(int a, int b) {
    return a < b ? EdgeKey{a, b} : EdgeKey{b, a};
}

int findOrCreateVertex(std::vector<MeshSimplifier::Vertex>& verts,
                       std::map<std::tuple<float,float,float>, int>& vertexMap,
                       const Vec3f& pos, int surfaceId) {
    auto key = std::make_tuple(pos.x(), pos.y(), pos.z());
    auto it = vertexMap.find(key);
    if (it != vertexMap.end()) return it->second;
    int idx = static_cast<int>(verts.size());
    MeshSimplifier::Vertex v;
    v.pos = pos;
    v.surfaceId = surfaceId;
    verts.push_back(v);
    vertexMap[key] = idx;
    return idx;
}

} // namespace

Eigen::Matrix4f MeshSimplifier::computePlaneQuadric(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
    Vec3f n = (v1 - v0).cross(v2 - v0);
    float area = n.norm();
    if (area < 1e-12f) return Eigen::Matrix4f::Zero();
    n /= area;
    float d = -n.dot(v0);
    Eigen::Vector4f p(n.x(), n.y(), n.z(), d);
    return p * p.transpose();
}

std::vector<Wall> MeshSimplifier::simplify(const std::vector<Wall>& walls,
                                            const std::vector<int>& surfaceIds,
                                            int targetCount) {
    if (static_cast<int>(walls.size()) <= targetCount)
        return walls;

    // Build indexed mesh
    std::vector<Vertex> verts;
    std::vector<Face> faces;
    std::map<std::tuple<float,float,float>, int> vertexMap;

    for (int i = 0; i < static_cast<int>(walls.size()); ++i) {
        const auto& tri = walls[i].triangle;
        int sid = (i < static_cast<int>(surfaceIds.size())) ? surfaceIds[i] : 0;
        int vi0 = findOrCreateVertex(verts, vertexMap, tri.v0, sid);
        int vi1 = findOrCreateVertex(verts, vertexMap, tri.v1, sid);
        int vi2 = findOrCreateVertex(verts, vertexMap, tri.v2, sid);

        Face f;
        f.v[0] = vi0; f.v[1] = vi1; f.v[2] = vi2;
        f.surfaceId = sid;
        f.energyAbsorption = walls[i].energyAbsorption;
        f.scattering = walls[i].scattering;
        faces.push_back(f);
    }

    // Build per-vertex face adjacency
    std::vector<std::unordered_set<int>> vertFaces(verts.size());
    for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
        for (int k = 0; k < 3; ++k)
            vertFaces[faces[fi].v[k]].insert(fi);
    }

    // Compute vertex quadrics from adjacent faces
    for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
        auto Q = computePlaneQuadric(
            verts[faces[fi].v[0]].pos,
            verts[faces[fi].v[1]].pos,
            verts[faces[fi].v[2]].pos);
        for (int k = 0; k < 3; ++k)
            verts[faces[fi].v[k]].quadric += Q;
    }

    // Detect boundary vertices (edges with only one adjacent face)
    std::unordered_map<EdgeKey, int, PairHash> edgeFaceCount;
    for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
        for (int k = 0; k < 3; ++k) {
            auto ek = makeEdgeKey(faces[fi].v[k], faces[fi].v[(k + 1) % 3]);
            edgeFaceCount[ek]++;
        }
    }
    for (auto& [ek, cnt] : edgeFaceCount) {
        if (cnt == 1) {
            verts[ek.first].boundary = true;
            verts[ek.second].boundary = true;
        }
    }

    // Mark vertices shared between multiple surface groups
    for (auto& v : verts) v.surfaceId = -1;
    for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
        for (int k = 0; k < 3; ++k) {
            int vi = faces[fi].v[k];
            if (verts[vi].surfaceId == -1)
                verts[vi].surfaceId = faces[fi].surfaceId;
            else if (verts[vi].surfaceId != faces[fi].surfaceId)
                verts[vi].surfaceId = -2; // mixed
        }
    }

    // Evaluate edge collapse cost
    auto evaluateCollapse = [&](int v0, int v1) -> EdgeCollapse {
        EdgeCollapse ec;
        ec.v0 = v0;
        ec.v1 = v1;
        ec.cost = std::numeric_limits<float>::max();

        // Forbid collapsing edges that cross surface boundaries
        if (verts[v0].surfaceId >= 0 && verts[v1].surfaceId >= 0 &&
            verts[v0].surfaceId != verts[v1].surfaceId) {
            return ec;
        }

        // Heavy penalty for boundary edges to preserve room shape
        float boundaryPenalty = 1.0f;
        if (verts[v0].boundary || verts[v1].boundary)
            boundaryPenalty = 100.0f;

        Eigen::Matrix4f Q = verts[v0].quadric + verts[v1].quadric;

        // Try to find optimal position via solving Q * v = 0
        Eigen::Matrix4f Qbar = Q;
        Qbar(3, 0) = 0; Qbar(3, 1) = 0; Qbar(3, 2) = 0; Qbar(3, 3) = 1;

        float det = Qbar.determinant();
        if (std::abs(det) > 1e-10f) {
            Eigen::Vector4f rhs(0, 0, 0, 1);
            Eigen::Vector4f opt = Qbar.inverse() * rhs;
            ec.optimalPos = Vec3f(opt.x(), opt.y(), opt.z());
        } else {
            ec.optimalPos = (verts[v0].pos + verts[v1].pos) * 0.5f;
        }

        Eigen::Vector4f vbar(ec.optimalPos.x(), ec.optimalPos.y(), ec.optimalPos.z(), 1.0f);
        ec.cost = std::max(0.0f, static_cast<float>(vbar.transpose() * Q * vbar)) * boundaryPenalty;
        return ec;
    };

    // Build priority queue
    std::priority_queue<EdgeCollapse, std::vector<EdgeCollapse>, std::greater<EdgeCollapse>> pq;
    std::unordered_set<EdgeKey, PairHash> insertedEdges;

    for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
        for (int k = 0; k < 3; ++k) {
            auto ek = makeEdgeKey(faces[fi].v[k], faces[fi].v[(k + 1) % 3]);
            if (insertedEdges.insert(ek).second) {
                auto ec = evaluateCollapse(ek.first, ek.second);
                if (ec.cost < std::numeric_limits<float>::max())
                    pq.push(ec);
            }
        }
    }

    // Iteratively collapse edges
    std::vector<int> remap(verts.size());
    std::iota(remap.begin(), remap.end(), 0);

    auto findRoot = [&](int v) {
        while (remap[v] != v) v = remap[v];
        return v;
    };

    int activeFaces = static_cast<int>(faces.size());

    while (activeFaces > targetCount && !pq.empty()) {
        auto ec = pq.top();
        pq.pop();

        int rv0 = findRoot(ec.v0);
        int rv1 = findRoot(ec.v1);
        if (rv0 == rv1) continue;

        // Collapse rv1 into rv0
        remap[rv1] = rv0;
        verts[rv0].pos = ec.optimalPos;
        verts[rv0].quadric = verts[rv0].quadric + verts[rv1].quadric;
        if (verts[rv1].boundary) verts[rv0].boundary = true;
        if (verts[rv0].surfaceId != verts[rv1].surfaceId)
            verts[rv0].surfaceId = -2;

        // Update faces
        std::unordered_set<EdgeKey, PairHash> newEdges;
        for (int fi : vertFaces[rv1]) {
            if (faces[fi].removed) continue;
            vertFaces[rv0].insert(fi);
        }

        for (int fi : vertFaces[rv0]) {
            if (faces[fi].removed) continue;
            for (int k = 0; k < 3; ++k)
                faces[fi].v[k] = findRoot(faces[fi].v[k]);

            // Degenerate face check
            if (faces[fi].v[0] == faces[fi].v[1] ||
                faces[fi].v[1] == faces[fi].v[2] ||
                faces[fi].v[2] == faces[fi].v[0]) {
                faces[fi].removed = true;
                activeFaces--;
                continue;
            }

            // Collect new edges for re-evaluation
            for (int k = 0; k < 3; ++k) {
                int a = faces[fi].v[k];
                int b = faces[fi].v[(k + 1) % 3];
                if (a == rv0 || b == rv0)
                    newEdges.insert(makeEdgeKey(a, b));
            }
        }

        for (auto& ek : newEdges) {
            auto newEc = evaluateCollapse(ek.first, ek.second);
            if (newEc.cost < std::numeric_limits<float>::max())
                pq.push(newEc);
        }
    }

    // Rebuild wall list from surviving faces
    std::vector<Wall> result;
    result.reserve(activeFaces);
    for (auto& f : faces) {
        if (f.removed) continue;
        int a = findRoot(f.v[0]);
        int b = findRoot(f.v[1]);
        int c = findRoot(f.v[2]);
        if (a == b || b == c || c == a) continue;

        Wall w;
        w.triangle.v0 = verts[a].pos;
        w.triangle.v1 = verts[b].pos;
        w.triangle.v2 = verts[c].pos;
        Vec3f e1 = w.triangle.v1 - w.triangle.v0;
        Vec3f e2 = w.triangle.v2 - w.triangle.v0;
        w.triangle.normal = e1.cross(e2).normalized();
        w.energyAbsorption = f.energyAbsorption;
        w.scattering = f.scattering;

        if (w.area() > 1e-8f)
            result.push_back(w);
    }

    return result;
}

} // namespace prs
