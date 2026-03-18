#include "DGMesh2D.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <numeric>
#include <cassert>

namespace prs {
namespace dg {

// ========== Room polygon extraction ==========

RoomPolygon2D extractRoomPolygon(
    const std::vector<Viewport3D::WallInfo>& walls,
    const std::vector<Vec3f>& modelVertices,
    float sliceHeight)
{
    // Intersect every triangle with the horizontal plane z = sliceHeight.
    // Collect all intersection segments, then chain them into a polygon.
    struct Seg { Vec2d a, b; double absorption; double scattering; };
    std::vector<Seg> segments;

    for (const auto& wi : walls) {
        for (int triIdx : wi.triangleIndices) {
            int base = triIdx * 3;
            if (base + 2 >= static_cast<int>(modelVertices.size())) continue;

            Vec3f verts[3] = {
                modelVertices[base],
                modelVertices[base + 1],
                modelVertices[base + 2]
            };

            // Classify each vertex relative to the slice plane
            double d[3];
            for (int i = 0; i < 3; ++i)
                d[i] = static_cast<double>(verts[i].z()) - sliceHeight;

            // Find intersection points (where edge crosses z = sliceHeight)
            std::vector<Vec2d> pts;
            for (int i = 0; i < 3; ++i) {
                int j = (i + 1) % 3;
                if ((d[i] > 0) != (d[j] > 0)) {
                    double t = d[i] / (d[i] - d[j]);
                    double ix = verts[i].x() + t * (verts[j].x() - verts[i].x());
                    double iy = verts[i].y() + t * (verts[j].y() - verts[i].y());
                    pts.push_back(Vec2d(ix, iy));
                } else if (std::abs(d[i]) < 1e-6) {
                    pts.push_back(Vec2d(verts[i].x(), verts[i].y()));
                }
            }
            // A plane-triangle intersection produces 0 or 2 points (or degenerate)
            if (pts.size() >= 2) {
                float avgAbs = 0.0f;
                for (float a : wi.absorption) avgAbs += a;
                avgAbs /= NUM_FREQ_BANDS;
                segments.push_back({pts[0], pts[1], static_cast<double>(avgAbs), static_cast<double>(wi.scattering)});
            }
        }
    }

    // If no intersections found, try projecting wall surfaces onto XY plane.
    // Fall back: use bounding box of all vertices projected to 2D.
    if (segments.empty()) {
        double minX = 1e30, maxX = -1e30, minY = 1e30, maxY = -1e30;
        for (const auto& v : modelVertices) {
            minX = std::min(minX, (double)v.x());
            maxX = std::max(maxX, (double)v.x());
            minY = std::min(minY, (double)v.y());
            maxY = std::max(maxY, (double)v.y());
        }
        double margin = 0.01;
        RoomPolygon2D poly;
        poly.vertices = {
            Vec2d(minX - margin, minY - margin),
            Vec2d(maxX + margin, minY - margin),
            Vec2d(maxX + margin, maxY + margin),
            Vec2d(minX - margin, maxY + margin)
        };
        poly.edgeAbsorption = {0.2, 0.2, 0.2, 0.2};
        poly.edgeScattering = {0.1, 0.1, 0.1, 0.1};
        return poly;
    }

    // Chain segments into an ordered polygon.
    // Use a greedy nearest-endpoint chaining approach.
    auto dist2 = [](const Vec2d& a, const Vec2d& b) { return (a - b).squaredNorm(); };
    double snapTol = 1e-4;

    std::vector<bool> used(segments.size(), false);
    std::vector<Vec2d> chain;
    std::vector<double> chainAbsorption;
    std::vector<double> chainScattering;

    // Start with the first segment
    used[0] = true;
    chain.push_back(segments[0].a);
    chain.push_back(segments[0].b);
    chainAbsorption.push_back(segments[0].absorption);
    chainScattering.push_back(segments[0].scattering);

    bool progress = true;
    while (progress) {
        progress = false;
        Vec2d tail = chain.back();
        double bestDist = snapTol * snapTol;
        int bestIdx = -1;
        bool bestFlip = false;

        for (int i = 0; i < static_cast<int>(segments.size()); ++i) {
            if (used[i]) continue;
            double da = dist2(tail, segments[i].a);
            double db = dist2(tail, segments[i].b);
            if (da < bestDist) { bestDist = da; bestIdx = i; bestFlip = false; }
            if (db < bestDist) { bestDist = db; bestIdx = i; bestFlip = true; }
        }

        if (bestIdx >= 0) {
            used[bestIdx] = true;
            Vec2d newPt = bestFlip ? segments[bestIdx].a : segments[bestIdx].b;
            chain.push_back(newPt);
            chainAbsorption.push_back(segments[bestIdx].absorption);
            chainScattering.push_back(segments[bestIdx].scattering);
            progress = true;
        }
    }

    // Close the polygon if start ~ end
    if (chain.size() > 2 && dist2(chain.front(), chain.back()) < snapTol * snapTol) {
        chain.pop_back();
        chainAbsorption.pop_back();
        chainScattering.pop_back();
    }

    // Remove near-duplicate consecutive vertices
    RoomPolygon2D poly;
    for (size_t i = 0; i < chain.size(); ++i) {
        if (poly.vertices.empty() ||
            (chain[i] - poly.vertices.back()).squaredNorm() > snapTol * snapTol * 0.01) {
            poly.vertices.push_back(chain[i]);
            if (i < chainAbsorption.size()) {
                poly.edgeAbsorption.push_back(chainAbsorption[i]);
                poly.edgeScattering.push_back(chainScattering[i]);
            }
        }
    }

    // Ensure we have at least 3 vertices
    if (poly.vertices.size() < 3) {
        double minX = 1e30, maxX = -1e30, minY = 1e30, maxY = -1e30;
        for (const auto& v : modelVertices) {
            minX = std::min(minX, (double)v.x());
            maxX = std::max(maxX, (double)v.x());
            minY = std::min(minY, (double)v.y());
            maxY = std::max(maxY, (double)v.y());
        }
        poly.vertices = {
            Vec2d(minX, minY), Vec2d(maxX, minY),
            Vec2d(maxX, maxY), Vec2d(minX, maxY)
        };
        poly.edgeAbsorption = {0.2, 0.2, 0.2, 0.2};
        poly.edgeScattering = {0.1, 0.1, 0.1, 0.1};
    }

    while (poly.edgeAbsorption.size() < poly.vertices.size())
        poly.edgeAbsorption.push_back(0.2);
    while (poly.edgeScattering.size() < poly.vertices.size())
        poly.edgeScattering.push_back(0.1);

    return poly;
}

// ========== Point-in-polygon test (ray casting) ==========

static bool pointInPolygon(const Vec2d& p, const std::vector<Vec2d>& poly) {
    int n = static_cast<int>(poly.size());
    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        if (((poly[i].y() > p.y()) != (poly[j].y() > p.y())) &&
            (p.x() < (poly[j].x() - poly[i].x()) * (p.y() - poly[i].y())
                      / (poly[j].y() - poly[i].y()) + poly[i].x())) {
            inside = !inside;
        }
    }
    return inside;
}

// ========== Bowyer-Watson Delaunay triangulation ==========

struct BWTriangle {
    int v[3];
    bool bad = false;
};

static double circumRadius2(const Vec2d& a, const Vec2d& b, const Vec2d& c) {
    double ax = a.x() - c.x(), ay = a.y() - c.y();
    double bx = b.x() - c.x(), by = b.y() - c.y();
    double D = 2.0 * (ax * by - ay * bx);
    if (std::abs(D) < 1e-30) return 1e30;
    double ux = (by * (ax * ax + ay * ay) - ay * (bx * bx + by * by)) / D;
    double uy = (ax * (bx * bx + by * by) - bx * (ax * ax + ay * ay)) / D;
    return ux * ux + uy * uy;
}

static Vec2d circumCenter(const Vec2d& a, const Vec2d& b, const Vec2d& c) {
    double ax = a.x() - c.x(), ay = a.y() - c.y();
    double bx = b.x() - c.x(), by = b.y() - c.y();
    double D = 2.0 * (ax * by - ay * bx);
    if (std::abs(D) < 1e-30) return (a + b + c) / 3.0;
    double ux = (by * (ax * ax + ay * ay) - ay * (bx * bx + by * by)) / D;
    double uy = (ax * (bx * bx + by * by) - bx * (ax * ax + ay * ay)) / D;
    return Vec2d(ux + c.x(), uy + c.y());
}

static bool inCircumcircle(const Vec2d& p, const Vec2d& a, const Vec2d& b, const Vec2d& c) {
    Vec2d cc = circumCenter(a, b, c);
    double r2 = (a - cc).squaredNorm();
    return (p - cc).squaredNorm() < r2 * (1.0 + 1e-10);
}

struct Edge2D {
    int a, b;
    bool operator==(const Edge2D& o) const {
        return (a == o.a && b == o.b) || (a == o.b && b == o.a);
    }
};

static std::vector<BWTriangle> bowyerWatson(
    const std::vector<Vec2d>& points, int nPts)
{
    // Super-triangle that contains all points
    double minX = 1e30, maxX = -1e30, minY = 1e30, maxY = -1e30;
    for (int i = 0; i < nPts; ++i) {
        minX = std::min(minX, points[i].x());
        maxX = std::max(maxX, points[i].x());
        minY = std::min(minY, points[i].y());
        maxY = std::max(maxY, points[i].y());
    }
    double dx = maxX - minX, dy = maxY - minY;
    double dmax = std::max(dx, dy);
    double midX = (minX + maxX) * 0.5, midY = (minY + maxY) * 0.5;

    // Super-triangle vertices stored at indices nPts, nPts+1, nPts+2
    // (caller must ensure 'points' has room for 3 extra)
    int s0 = nPts, s1 = nPts + 1, s2 = nPts + 2;

    std::vector<BWTriangle> triangles;
    triangles.push_back({{s0, s1, s2}});

    for (int i = 0; i < nPts; ++i) {
        const Vec2d& p = points[i];

        // Find all triangles whose circumcircle contains p
        for (auto& tri : triangles) {
            if (inCircumcircle(p, points[tri.v[0]], points[tri.v[1]], points[tri.v[2]]))
                tri.bad = true;
        }

        // Find boundary of the polygonal hole
        std::vector<Edge2D> polygon;
        for (const auto& tri : triangles) {
            if (!tri.bad) continue;
            for (int e = 0; e < 3; ++e) {
                Edge2D edge = {tri.v[e], tri.v[(e + 1) % 3]};
                bool shared = false;
                for (const auto& other : triangles) {
                    if (&other == &tri || !other.bad) continue;
                    for (int oe = 0; oe < 3; ++oe) {
                        Edge2D otherEdge = {other.v[oe], other.v[(oe + 1) % 3]};
                        if (edge == otherEdge) { shared = true; break; }
                    }
                    if (shared) break;
                }
                if (!shared) polygon.push_back(edge);
            }
        }

        // Remove bad triangles
        triangles.erase(
            std::remove_if(triangles.begin(), triangles.end(),
                          [](const BWTriangle& t) { return t.bad; }),
            triangles.end());

        // Re-triangulate the hole
        for (const auto& edge : polygon) {
            triangles.push_back({{edge.a, edge.b, i}});
        }
    }

    // Remove triangles that use super-triangle vertices
    triangles.erase(
        std::remove_if(triangles.begin(), triangles.end(),
                      [s0, s1, s2](const BWTriangle& t) {
                          for (int v : t.v)
                              if (v == s0 || v == s1 || v == s2) return true;
                          return false;
                      }),
        triangles.end());

    return triangles;
}

// ========== Assign absorption to boundary edges ==========

static int findNearestEdge(const Vec2d& mid, const std::vector<Vec2d>& polyVerts) {
    int nEdges = static_cast<int>(polyVerts.size());
    int best = 0;
    double bestDist = 1e30;
    for (int i = 0; i < nEdges; ++i) {
        int j = (i + 1) % nEdges;
        Vec2d edgeMid = (polyVerts[i] + polyVerts[j]) * 0.5;
        double d = (mid - edgeMid).squaredNorm();

        // Also check distance to the edge segment itself
        Vec2d ab = polyVerts[j] - polyVerts[i];
        double len2 = ab.squaredNorm();
        if (len2 > 1e-20) {
            double t = std::clamp((mid - polyVerts[i]).dot(ab) / len2, 0.0, 1.0);
            Vec2d closest = polyVerts[i] + t * ab;
            d = std::min(d, (mid - closest).squaredNorm());
        }

        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

// ========== Build the full 2D DG mesh ==========

Mesh2D buildMesh2D(
    const RoomPolygon2D& polygon,
    const Basis2D& basis,
    double targetElementSize)
{
    Mesh2D mesh;
    int N = basis.N;
    int Np = basis.Np;
    int Nfp = basis.Nfp;
    int Nfaces = basis.Nfaces;

    double h = targetElementSize;
    const auto& polyVerts = polygon.vertices;
    int nPolyVerts = static_cast<int>(polyVerts.size());

    // Step 1: Generate boundary points along polygon edges
    std::vector<Vec2d> allPoints;
    std::vector<int> boundaryPointEdge; // which polygon edge each boundary point belongs to

    for (int i = 0; i < nPolyVerts; ++i) {
        int j = (i + 1) % nPolyVerts;
        Vec2d a = polyVerts[i], b = polyVerts[j];
        double edgeLen = (b - a).norm();
        int nSeg = std::max(1, static_cast<int>(std::ceil(edgeLen / h)));
        for (int k = 0; k < nSeg; ++k) {
            double t = static_cast<double>(k) / nSeg;
            allPoints.push_back(a + t * (b - a));
            boundaryPointEdge.push_back(i);
        }
    }
    int nBoundaryPts = static_cast<int>(allPoints.size());

    // Step 2: Generate interior points on a regular grid
    double minX = 1e30, maxX = -1e30, minY = 1e30, maxY = -1e30;
    for (const auto& v : polyVerts) {
        minX = std::min(minX, v.x()); maxX = std::max(maxX, v.x());
        minY = std::min(minY, v.y()); maxY = std::max(maxY, v.y());
    }

    double offset = h * 0.5;
    for (double yy = minY + offset; yy < maxY; yy += h * std::sqrt(3.0) / 2.0) {
        int row = static_cast<int>((yy - minY) / (h * std::sqrt(3.0) / 2.0));
        double xoff = (row % 2 == 0) ? 0.0 : h * 0.5;
        for (double xx = minX + offset + xoff; xx < maxX; xx += h) {
            Vec2d p(xx, yy);
            if (pointInPolygon(p, polyVerts)) {
                bool tooClose = false;
                for (int i = 0; i < nBoundaryPts; ++i) {
                    if ((p - allPoints[i]).squaredNorm() < h * h * 0.25) {
                        tooClose = true;
                        break;
                    }
                }
                if (!tooClose) allPoints.push_back(p);
            }
        }
    }

    int nPts = static_cast<int>(allPoints.size());

    // Step 3: Add super-triangle vertices
    double dmax = std::max(maxX - minX, maxY - minY);
    double cx = (minX + maxX) * 0.5, cy = (minY + maxY) * 0.5;
    allPoints.push_back(Vec2d(cx - 20 * dmax, cy - 10 * dmax));
    allPoints.push_back(Vec2d(cx + 20 * dmax, cy - 10 * dmax));
    allPoints.push_back(Vec2d(cx, cy + 20 * dmax));

    // Step 4: Bowyer-Watson triangulation
    auto triangles = bowyerWatson(allPoints, nPts);

    // Step 5: Remove triangles with centroids outside the polygon
    std::vector<BWTriangle> validTriangles;
    for (const auto& tri : triangles) {
        Vec2d centroid = (allPoints[tri.v[0]] + allPoints[tri.v[1]] + allPoints[tri.v[2]]) / 3.0;
        if (pointInPolygon(centroid, polyVerts))
            validTriangles.push_back(tri);
    }

    mesh.K = static_cast<int>(validTriangles.size());
    if (mesh.K == 0) {
        // Fallback: single triangle covering bounding box
        allPoints.clear();
        allPoints.push_back(Vec2d(minX, minY));
        allPoints.push_back(Vec2d(maxX, minY));
        allPoints.push_back(Vec2d(minX, maxY));
        validTriangles.clear();
        validTriangles.push_back({{0, 1, 2}});
        mesh.K = 1;
        nPts = 3;
    }

    // Step 6: Build vertex array (only used vertices)
    std::map<int, int> vertexRemap;
    std::vector<Vec2d> usedVertices;
    for (const auto& tri : validTriangles) {
        for (int v : tri.v) {
            if (vertexRemap.find(v) == vertexRemap.end()) {
                vertexRemap[v] = static_cast<int>(usedVertices.size());
                usedVertices.push_back(allPoints[v]);
            }
        }
    }

    mesh.Nv = static_cast<int>(usedVertices.size());
    mesh.VX.resize(mesh.Nv);
    mesh.VY.resize(mesh.Nv);
    for (int i = 0; i < mesh.Nv; ++i) {
        mesh.VX(i) = usedVertices[i].x();
        mesh.VY(i) = usedVertices[i].y();
    }

    // Step 7: Element-to-vertex connectivity
    mesh.EToV.resize(mesh.K, 3);
    for (int k = 0; k < mesh.K; ++k) {
        for (int j = 0; j < 3; ++j)
            mesh.EToV(k, j) = vertexRemap[validTriangles[k].v[j]];

        // Ensure consistent (counter-clockwise) orientation
        Vec2d v0(mesh.VX(mesh.EToV(k, 0)), mesh.VY(mesh.EToV(k, 0)));
        Vec2d v1(mesh.VX(mesh.EToV(k, 1)), mesh.VY(mesh.EToV(k, 1)));
        Vec2d v2(mesh.VX(mesh.EToV(k, 2)), mesh.VY(mesh.EToV(k, 2)));
        double cross = (v1.x() - v0.x()) * (v2.y() - v0.y())
                      - (v2.x() - v0.x()) * (v1.y() - v0.y());
        if (cross < 0) std::swap(mesh.EToV(k, 1), mesh.EToV(k, 2));
    }

    // Step 8: Build high-order node coordinates via affine mapping
    mesh.x.resize(Np, mesh.K);
    mesh.y.resize(Np, mesh.K);

    for (int k = 0; k < mesh.K; ++k) {
        double x0 = mesh.VX(mesh.EToV(k, 0)), y0 = mesh.VY(mesh.EToV(k, 0));
        double x1 = mesh.VX(mesh.EToV(k, 1)), y1 = mesh.VY(mesh.EToV(k, 1));
        double x2 = mesh.VX(mesh.EToV(k, 2)), y2 = mesh.VY(mesh.EToV(k, 2));

        // Reference triangle: (-1,-1), (1,-1), (-1,1)
        // Physical: v0 + (v1-v0)*(1+r)/2 + (v2-v0)*(1+s)/2
        for (int i = 0; i < Np; ++i) {
            double r = basis.r(i), s = basis.s(i);
            mesh.x(i, k) = 0.5 * (-(r + s) * x0 + (1.0 + r) * x1 + (1.0 + s) * x2);
            mesh.y(i, k) = 0.5 * (-(r + s) * y0 + (1.0 + r) * y1 + (1.0 + s) * y2);
        }
    }

    // Step 9: Geometric factors
    mesh.rx.resize(Np, mesh.K);
    mesh.ry.resize(Np, mesh.K);
    mesh.sx.resize(Np, mesh.K);
    mesh.sy.resize(Np, mesh.K);
    mesh.J.resize(Np, mesh.K);

    // For affine (straight-sided) elements, geometric factors are constant per element
    // but we store per node for generality.
    MatXd xr = basis.Dr * mesh.x;  // dx/dr
    MatXd xs = basis.Ds * mesh.x;  // dx/ds
    MatXd yr = basis.Dr * mesh.y;  // dy/dr
    MatXd ys = basis.Ds * mesh.y;  // dy/ds

    mesh.J = xr.cwiseProduct(ys) - xs.cwiseProduct(yr);
    mesh.rx = ys.cwiseQuotient(mesh.J);
    mesh.ry = (-xs).cwiseQuotient(mesh.J);
    mesh.sx = (-yr).cwiseQuotient(mesh.J);
    mesh.sy = xr.cwiseQuotient(mesh.J);

    // Step 10: Build connectivity (EToE, EToF)
    mesh.EToE.resize(mesh.K, Nfaces);
    mesh.EToF.resize(mesh.K, Nfaces);

    // Initialize to self (boundary)
    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            mesh.EToE(k, f) = k;
            mesh.EToF(k, f) = f;
        }
    }

    // Face vertices for each face of each element (using EToV vertex indices)
    // Face 0: edge v0-v1 (s=-1), Face 1: edge v1-v2 (r+s=0), Face 2: edge v2-v0 (r=-1)
    using FaceKey = std::pair<int, int>;
    auto makeFaceKey = [](int a, int b) -> FaceKey {
        return (a < b) ? FaceKey{a, b} : FaceKey{b, a};
    };

    std::map<FaceKey, std::pair<int, int>> faceMap; // key -> (element, localFace)

    for (int k = 0; k < mesh.K; ++k) {
        int v0 = mesh.EToV(k, 0), v1 = mesh.EToV(k, 1), v2 = mesh.EToV(k, 2);
        FaceKey faces[3] = {
            makeFaceKey(v0, v1),
            makeFaceKey(v1, v2),
            makeFaceKey(v2, v0)
        };
        for (int f = 0; f < Nfaces; ++f) {
            auto it = faceMap.find(faces[f]);
            if (it != faceMap.end()) {
                int k2 = it->second.first;
                int f2 = it->second.second;
                mesh.EToE(k, f) = k2;
                mesh.EToF(k, f) = f2;
                mesh.EToE(k2, f2) = k;
                mesh.EToF(k2, f2) = f;
            } else {
                faceMap[faces[f]] = {k, f};
            }
        }
    }

    // Step 11: Surface normals and Jacobians
    int totalFaceNodes = Nfp * Nfaces;
    mesh.nx.resize(totalFaceNodes, mesh.K);
    mesh.ny.resize(totalFaceNodes, mesh.K);
    mesh.sJ.resize(totalFaceNodes, mesh.K);
    mesh.Fscale.resize(totalFaceNodes, mesh.K);

    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            for (int i = 0; i < Nfp; ++i) {
                int fid = f * Nfp + i;
                int nodeIdx = basis.Fmask(i, f);

                double rxv = mesh.rx(nodeIdx, k);
                double ryv = mesh.ry(nodeIdx, k);
                double sxv = mesh.sx(nodeIdx, k);
                double syv = mesh.sy(nodeIdx, k);

                double fnx, fny;
                if (f == 0) {
                    // Face 1: s = -1, outward normal = -(ds/dx, ds/dy) direction
                    fnx = -sxv;
                    fny = -syv;
                } else if (f == 1) {
                    // Face 2: r+s = 0, outward normal in direction (dr+ds)/dx, (dr+ds)/dy
                    fnx = rxv + sxv;
                    fny = ryv + syv;
                } else {
                    // Face 3: r = -1, outward normal = -(dr/dx, dr/dy)
                    fnx = -rxv;
                    fny = -ryv;
                }

                double mag = std::sqrt(fnx * fnx + fny * fny);
                if (mag > 1e-30) { fnx /= mag; fny /= mag; }

                // Surface Jacobian: |ds/dx| * |J| for face normal direction
                // For straight-sided elements: sJ = edge_length / 2
                mesh.nx(fid, k) = fnx;
                mesh.ny(fid, k) = fny;
                mesh.sJ(fid, k) = mag * std::abs(mesh.J(nodeIdx, k));
                mesh.Fscale(fid, k) = mesh.sJ(fid, k) / std::abs(mesh.J(nodeIdx, k));
            }
        }
    }

    // Step 12: Build node-level connectivity maps (vmapM, vmapP)
    int totalSurfNodes = totalFaceNodes * mesh.K;
    mesh.vmapM.resize(totalSurfNodes);
    mesh.vmapP.resize(totalSurfNodes);

    // vmapM: map surface node to volume node index (global = nodeIdx + k * Np)
    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            for (int i = 0; i < Nfp; ++i) {
                int surfIdx = k * totalFaceNodes + f * Nfp + i;
                int nodeIdx = basis.Fmask(i, f);
                mesh.vmapM(surfIdx) = k * Np + nodeIdx;
            }
        }
    }

    // vmapP: map to the matching node on the neighbor element
    mesh.vmapP = mesh.vmapM; // default: map to self (boundary)
    double matchTol = h * 1e-6;

    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            int k2 = mesh.EToE(k, f);
            int f2 = mesh.EToF(k, f);
            if (k2 == k && f2 == f) continue; // boundary

            for (int i = 0; i < Nfp; ++i) {
                int surfIdxM = k * totalFaceNodes + f * Nfp + i;
                int nodeM = basis.Fmask(i, f);
                double xM = mesh.x(nodeM, k), yM = mesh.y(nodeM, k);

                for (int j = 0; j < Nfp; ++j) {
                    int nodeP = basis.Fmask(j, f2);
                    double xP = mesh.x(nodeP, k2), yP = mesh.y(nodeP, k2);
                    if ((xM - xP) * (xM - xP) + (yM - yP) * (yM - yP) < matchTol * matchTol) {
                        mesh.vmapP(surfIdxM) = k2 * Np + nodeP;
                        break;
                    }
                }
            }
        }
    }

    // Step 13: Identify boundary nodes and assign impedance
    std::vector<int> boundaryIndices;
    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            if (mesh.EToE(k, f) == k && mesh.EToF(k, f) == f) {
                // This is a boundary face
                // Find the nearest polygon edge to assign material
                int nodeIdx = basis.Fmask(0, f);
                Vec2d faceMid(mesh.x(nodeIdx, k), mesh.y(nodeIdx, k));
                if (Nfp > 1) {
                    int nodeIdx2 = basis.Fmask(Nfp - 1, f);
                    faceMid = Vec2d(
                        (mesh.x(nodeIdx, k) + mesh.x(nodeIdx2, k)) * 0.5,
                        (mesh.y(nodeIdx, k) + mesh.y(nodeIdx2, k)) * 0.5);
                }

                int edgeIdx = findNearestEdge(faceMid, polyVerts);
                double alpha = polygon.edgeAbsorption[edgeIdx];
                double Z = absorptionToImpedance(alpha);

                BoundaryEdge2D be;
                be.element = k;
                be.localFace = f;
                be.impedance = Z;
                mesh.boundaryEdges.push_back(be);

                for (int i = 0; i < Nfp; ++i) {
                    int surfIdx = k * totalFaceNodes + f * Nfp + i;
                    boundaryIndices.push_back(surfIdx);
                }
            }
        }
    }

    mesh.mapB.resize(static_cast<int>(boundaryIndices.size()));
    for (int i = 0; i < static_cast<int>(boundaryIndices.size()); ++i)
        mesh.mapB(i) = boundaryIndices[i];

    // Build per-surface-node impedance array (for all surface nodes, interior = 0)
    mesh.boundaryImpedance = VecXd::Zero(totalSurfNodes);
    for (const auto& be : mesh.boundaryEdges) {
        for (int i = 0; i < Nfp; ++i) {
            int surfIdx = be.element * totalFaceNodes + be.localFace * Nfp + i;
            mesh.boundaryImpedance(surfIdx) = be.impedance;
        }
    }

    mesh.bcType = VecXi::Zero(totalSurfNodes);
    for (int i = 0; i < mesh.mapB.size(); ++i)
        mesh.bcType(mesh.mapB(i)) = 1; // 1 = impedance BC

    return mesh;
}

} // namespace dg
} // namespace prs
