#include "DGMesh3D.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>
#include <set>

namespace prs {
namespace dg {

// Point-inside-mesh test using ray casting against BVH
static bool pointInsideMesh(const Vec3f& point, const Bvh& bvh) {
    // Cast a ray in +X direction and count intersections
    Vec3f dir(1.0f, 0.0f, 0.0f);
    int crossings = 0;
    float tMin = 1e-6f;

    // Walk along the ray, counting all hits
    Vec3f origin = point;
    for (int iter = 0; iter < 1000; ++iter) {
        BvhHit hit = bvh.closestHit(origin, dir, tMin);
        if (hit.wallIndex < 0)
            break;
        crossings++;
        origin = origin + dir * (hit.t + tMin);
    }

    return (crossings % 2) == 1;
}

// Find nearest wall to a point for material assignment
static int findNearestWall(const Vec3f& point, const std::vector<Viewport3D::WallInfo>& walls,
                           const std::vector<Vec3f>& modelVertices) {
    int bestWall = 0;
    float bestDist = 1e30f;

    for (int w = 0; w < static_cast<int>(walls.size()); ++w) {
        for (int triIdx : walls[w].triangleIndices) {
            int base = triIdx * 3;
            if (base + 2 >= static_cast<int>(modelVertices.size()))
                continue;

            Vec3f centroid = (modelVertices[base] + modelVertices[base + 1] + modelVertices[base + 2]) / 3.0f;
            float d = (point - centroid).squaredNorm();
            if (d < bestDist) {
                bestDist = d;
                bestWall = w;
            }
        }
    }
    return bestWall;
}

Mesh3D buildMesh3D(const std::vector<Viewport3D::WallInfo>& walls, const std::vector<Vec3f>& modelVertices,
                   const Bvh& bvh, const Basis3D& basis, double targetElementSize) {
    Mesh3D mesh;
    int N = basis.N;
    int Np = basis.Np;
    int Nfp = basis.Nfp;
    int Nfaces = basis.Nfaces;
    double h = targetElementSize;

    // Step 1: Compute bounding box
    float minX = 1e30f, maxX = -1e30f;
    float minY = 1e30f, maxY = -1e30f;
    float minZ = 1e30f, maxZ = -1e30f;
    for (const auto& v : modelVertices) {
        minX = std::min(minX, v.x());
        maxX = std::max(maxX, v.x());
        minY = std::min(minY, v.y());
        maxY = std::max(maxY, v.y());
        minZ = std::min(minZ, v.z());
        maxZ = std::max(maxZ, v.z());
    }

    // Small inward margin so boundary tets are properly inside
    float margin = static_cast<float>(h * 0.1);
    minX += margin;
    maxX -= margin;
    minY += margin;
    maxY -= margin;
    minZ += margin;
    maxZ -= margin;

    // Step 2: Create regular grid of cubes
    int nx = std::max(2, static_cast<int>(std::ceil((maxX - minX) / h)));
    int ny = std::max(2, static_cast<int>(std::ceil((maxY - minY) / h)));
    int nz = std::max(2, static_cast<int>(std::ceil((maxZ - minZ) / h)));

    double dx = (maxX - minX) / nx;
    double dy = (maxY - minY) / ny;
    double dz = (maxZ - minZ) / nz;

    // Grid vertices
    int nVerts = (nx + 1) * (ny + 1) * (nz + 1);
    std::vector<Vec3d> gridVerts(nVerts);
    auto gridIdx = [&](int ix, int iy, int iz) { return ix + (nx + 1) * (iy + (ny + 1) * iz); };

    for (int iz = 0; iz <= nz; ++iz)
        for (int iy = 0; iy <= ny; ++iy)
            for (int ix = 0; ix <= nx; ++ix) {
                gridVerts[gridIdx(ix, iy, iz)] = Vec3d(minX + ix * dx, minY + iy * dy, minZ + iz * dz);
            }

    // Step 3: Split each cube into 6 tetrahedra and test inside/outside
    // Standard decomposition of a cube into 6 tetrahedra:
    // Cube vertices labeled 0-7:
    // 0=(0,0,0), 1=(1,0,0), 2=(1,1,0), 3=(0,1,0),
    // 4=(0,0,1), 5=(1,0,1), 6=(1,1,1), 7=(0,1,1)
    static const int tetDecomp[6][4] = {
        {0, 1, 3, 4}, {1, 2, 3, 6}, {1, 4, 5, 6}, {3, 4, 6, 7}, {1, 3, 4, 6}, {0, 0, 0, 0} // 5 tets actually used
    };
    // Actually the standard 5-tet decomposition:
    static const int tet5[5][4] = {{0, 1, 3, 5}, {1, 2, 3, 5}, {2, 3, 5, 6}, {0, 3, 4, 5}, {3, 4, 5, 7}};
    // Or the more symmetric 6-tet decomposition using diagonal:
    static const int tet6[6][4] = {{0, 5, 1, 3}, {5, 1, 3, 2}, {5, 2, 3, 6}, {0, 5, 3, 4}, {5, 3, 4, 7}, {5, 6, 3, 7}};

    struct TetInfo {
        int v[4];
        bool inside;
    };
    std::vector<TetInfo> allTets;

    for (int iz = 0; iz < nz; ++iz) {
        for (int iy = 0; iy < ny; ++iy) {
            for (int ix = 0; ix < nx; ++ix) {
                // Cube vertex indices
                int cv[8] = {gridIdx(ix, iy, iz),
                             gridIdx(ix + 1, iy, iz),
                             gridIdx(ix + 1, iy + 1, iz),
                             gridIdx(ix, iy + 1, iz),
                             gridIdx(ix, iy, iz + 1),
                             gridIdx(ix + 1, iy, iz + 1),
                             gridIdx(ix + 1, iy + 1, iz + 1),
                             gridIdx(ix, iy + 1, iz + 1)};

                // Split into 6 tetrahedra
                for (int ti = 0; ti < 6; ++ti) {
                    TetInfo tet;
                    tet.v[0] = cv[tet6[ti][0]];
                    tet.v[1] = cv[tet6[ti][1]];
                    tet.v[2] = cv[tet6[ti][2]];
                    tet.v[3] = cv[tet6[ti][3]];

                    // Test centroid
                    Vec3d centroid =
                        (gridVerts[tet.v[0]] + gridVerts[tet.v[1]] + gridVerts[tet.v[2]] + gridVerts[tet.v[3]]) / 4.0;
                    Vec3f centF(static_cast<float>(centroid.x()), static_cast<float>(centroid.y()),
                                static_cast<float>(centroid.z()));
                    tet.inside = pointInsideMesh(centF, bvh);

                    if (tet.inside)
                        allTets.push_back(tet);
                }
            }
        }
    }

    mesh.K = static_cast<int>(allTets.size());
    if (mesh.K == 0) {
        // Fallback: create a single tet from bounding box
        mesh.K = 1;
        mesh.Nv = 4;
        mesh.VX.resize(4);
        mesh.VY.resize(4);
        mesh.VZ.resize(4);
        mesh.VX << minX, maxX, minX, minX;
        mesh.VY << minY, minY, maxY, minY;
        mesh.VZ << minZ, minZ, minZ, maxZ;
        mesh.EToV.resize(1, 4);
        mesh.EToV << 0, 1, 2, 3;
        // Fill remaining fields minimally below...
    }

    // Step 4: Build vertex and connectivity arrays
    if (mesh.K > 1 || allTets.size() > 0) {
        std::map<int, int> vertexRemap;
        std::vector<Vec3d> usedVerts;
        for (const auto& tet : allTets) {
            for (int v : tet.v) {
                if (vertexRemap.find(v) == vertexRemap.end()) {
                    vertexRemap[v] = static_cast<int>(usedVerts.size());
                    usedVerts.push_back(gridVerts[v]);
                }
            }
        }

        mesh.Nv = static_cast<int>(usedVerts.size());
        mesh.VX.resize(mesh.Nv);
        mesh.VY.resize(mesh.Nv);
        mesh.VZ.resize(mesh.Nv);
        for (int i = 0; i < mesh.Nv; ++i) {
            mesh.VX(i) = usedVerts[i].x();
            mesh.VY(i) = usedVerts[i].y();
            mesh.VZ(i) = usedVerts[i].z();
        }

        mesh.K = static_cast<int>(allTets.size());
        mesh.EToV.resize(mesh.K, 4);
        for (int k = 0; k < mesh.K; ++k) {
            for (int j = 0; j < 4; ++j)
                mesh.EToV(k, j) = vertexRemap[allTets[k].v[j]];

            // Ensure positive volume (right-hand orientation)
            Vec3d v0(mesh.VX(mesh.EToV(k, 0)), mesh.VY(mesh.EToV(k, 0)), mesh.VZ(mesh.EToV(k, 0)));
            Vec3d v1(mesh.VX(mesh.EToV(k, 1)), mesh.VY(mesh.EToV(k, 1)), mesh.VZ(mesh.EToV(k, 1)));
            Vec3d v2(mesh.VX(mesh.EToV(k, 2)), mesh.VY(mesh.EToV(k, 2)), mesh.VZ(mesh.EToV(k, 2)));
            Vec3d v3(mesh.VX(mesh.EToV(k, 3)), mesh.VY(mesh.EToV(k, 3)), mesh.VZ(mesh.EToV(k, 3)));
            double vol = (v1 - v0).dot((v2 - v0).cross(v3 - v0));
            if (vol < 0)
                std::swap(mesh.EToV(k, 2), mesh.EToV(k, 3));
        }
    }

    // Step 5: High-order node coordinates via affine mapping
    mesh.x.resize(Np, mesh.K);
    mesh.y.resize(Np, mesh.K);
    mesh.z.resize(Np, mesh.K);

    for (int k = 0; k < mesh.K; ++k) {
        double x0 = mesh.VX(mesh.EToV(k, 0)), y0 = mesh.VY(mesh.EToV(k, 0)), z0 = mesh.VZ(mesh.EToV(k, 0));
        double x1 = mesh.VX(mesh.EToV(k, 1)), y1 = mesh.VY(mesh.EToV(k, 1)), z1 = mesh.VZ(mesh.EToV(k, 1));
        double x2 = mesh.VX(mesh.EToV(k, 2)), y2 = mesh.VY(mesh.EToV(k, 2)), z2 = mesh.VZ(mesh.EToV(k, 2));
        double x3 = mesh.VX(mesh.EToV(k, 3)), y3 = mesh.VY(mesh.EToV(k, 3)), z3 = mesh.VZ(mesh.EToV(k, 3));

        // Reference tet: (-1,-1,-1), (1,-1,-1), (-1,1,-1), (-1,-1,1)
        // x = 0.5 * (-(r+s+t+1)*x0 + (1+r)*x1 + (1+s)*x2 + (1+t)*x3)
        for (int i = 0; i < Np; ++i) {
            double rv = basis.r(i), sv = basis.s(i), tv = basis.t(i);
            double L0 = -(rv + sv + tv + 1.0) * 0.5;
            double L1 = (1.0 + rv) * 0.5;
            double L2 = (1.0 + sv) * 0.5;
            double L3 = (1.0 + tv) * 0.5;
            // Note: L0 + L1 + L2 + L3 = 0.5*(-r-s-t-1+1+r+1+s+1+t) = 0.5*2 = 1 ✓
            mesh.x(i, k) = L0 * x0 + L1 * x1 + L2 * x2 + L3 * x3;
            mesh.y(i, k) = L0 * y0 + L1 * y1 + L2 * y2 + L3 * y3;
            mesh.z(i, k) = L0 * z0 + L1 * z1 + L2 * z2 + L3 * z3;
        }
    }

    // Step 6: Geometric factors
    mesh.rx.resize(Np, mesh.K);
    mesh.ry.resize(Np, mesh.K);
    mesh.rz.resize(Np, mesh.K);
    mesh.sx.resize(Np, mesh.K);
    mesh.sy.resize(Np, mesh.K);
    mesh.sz.resize(Np, mesh.K);
    mesh.tx.resize(Np, mesh.K);
    mesh.ty.resize(Np, mesh.K);
    mesh.tz.resize(Np, mesh.K);
    mesh.J.resize(Np, mesh.K);

    MatXd xr = basis.Dr * mesh.x, xs = basis.Ds * mesh.x, xt = basis.Dt * mesh.x;
    MatXd yr = basis.Dr * mesh.y, ys = basis.Ds * mesh.y, yt = basis.Dt * mesh.y;
    MatXd zr = basis.Dr * mesh.z, zs = basis.Ds * mesh.z, zt = basis.Dt * mesh.z;

    // Jacobian: J = [xr xs xt; yr ys yt; zr zs zt]
    // J = xr*(ys*zt - zs*yt) - xs*(yr*zt - zr*yt) + xt*(yr*zs - zr*ys)
    mesh.J = xr.cwiseProduct(ys.cwiseProduct(zt) - zs.cwiseProduct(yt)) -
             xs.cwiseProduct(yr.cwiseProduct(zt) - zr.cwiseProduct(yt)) +
             xt.cwiseProduct(yr.cwiseProduct(zs) - zr.cwiseProduct(ys));

    // Inverse Jacobian (cofactor matrix / J)
    mesh.rx = (ys.cwiseProduct(zt) - zs.cwiseProduct(yt)).cwiseQuotient(mesh.J);
    mesh.ry = (xt.cwiseProduct(zs) - xs.cwiseProduct(zt)).cwiseQuotient(mesh.J);
    mesh.rz = (xs.cwiseProduct(yt) - xt.cwiseProduct(ys)).cwiseQuotient(mesh.J);
    mesh.sx = (zt.cwiseProduct(yr) - yt.cwiseProduct(zr)).cwiseQuotient(mesh.J);
    mesh.sy = (xr.cwiseProduct(zt) - xt.cwiseProduct(zr)).cwiseQuotient(mesh.J);
    mesh.sz = (xt.cwiseProduct(yr) - xr.cwiseProduct(yt)).cwiseQuotient(mesh.J);
    mesh.tx = (yr.cwiseProduct(zs) - ys.cwiseProduct(zr)).cwiseQuotient(mesh.J);
    mesh.ty = (xs.cwiseProduct(zr) - xr.cwiseProduct(zs)).cwiseQuotient(mesh.J);
    mesh.tz = (xr.cwiseProduct(ys) - xs.cwiseProduct(yr)).cwiseQuotient(mesh.J);

    // Step 7: Build connectivity (EToE, EToF)
    mesh.EToE.resize(mesh.K, Nfaces);
    mesh.EToF.resize(mesh.K, Nfaces);
    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            mesh.EToE(k, f) = k;
            mesh.EToF(k, f) = f;
        }
    }

    // Face-to-vertex mapping for tetrahedron:
    // Face 0 (t=-1): v0, v1, v2
    // Face 1 (s=-1): v0, v1, v3
    // Face 2 (r+s+t=-1): v1, v2, v3
    // Face 3 (r=-1): v0, v2, v3
    static const int faceVerts[4][3] = {{0, 1, 2}, {0, 1, 3}, {1, 2, 3}, {0, 2, 3}};

    using FaceKey = std::array<int, 3>;
    auto makeFaceKey = [](int a, int b, int c) -> FaceKey {
        FaceKey k = {a, b, c};
        std::sort(k.begin(), k.end());
        return k;
    };

    std::map<FaceKey, std::pair<int, int>> faceMap;
    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            int va = mesh.EToV(k, faceVerts[f][0]);
            int vb = mesh.EToV(k, faceVerts[f][1]);
            int vc = mesh.EToV(k, faceVerts[f][2]);
            FaceKey key = makeFaceKey(va, vb, vc);

            auto it = faceMap.find(key);
            if (it != faceMap.end()) {
                int k2 = it->second.first, f2 = it->second.second;
                mesh.EToE(k, f) = k2;
                mesh.EToF(k, f) = f2;
                mesh.EToE(k2, f2) = k;
                mesh.EToF(k2, f2) = f;
            } else {
                faceMap[key] = {k, f};
            }
        }
    }

    // Step 8: Surface normals and Jacobians
    int totalFaceNodes = Nfp * Nfaces;
    mesh.nx.resize(totalFaceNodes, mesh.K);
    mesh.ny.resize(totalFaceNodes, mesh.K);
    mesh.nz.resize(totalFaceNodes, mesh.K);
    mesh.sJ.resize(totalFaceNodes, mesh.K);
    mesh.Fscale.resize(totalFaceNodes, mesh.K);

    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            for (int i = 0; i < Nfp; ++i) {
                int fid = f * Nfp + i;
                int nid = basis.Fmask(i, f);

                double rxv = mesh.rx(nid, k), ryv = mesh.ry(nid, k), rzv = mesh.rz(nid, k);
                double sxv = mesh.sx(nid, k), syv = mesh.sy(nid, k), szv = mesh.sz(nid, k);
                double txv = mesh.tx(nid, k), tyv = mesh.ty(nid, k), tzv = mesh.tz(nid, k);

                double fnx, fny, fnz;
                if (f == 0) { // t = -1
                    fnx = -txv;
                    fny = -tyv;
                    fnz = -tzv;
                } else if (f == 1) { // s = -1
                    fnx = -sxv;
                    fny = -syv;
                    fnz = -szv;
                } else if (f == 2) { // r+s+t = -1
                    fnx = rxv + sxv + txv;
                    fny = ryv + syv + tyv;
                    fnz = rzv + szv + tzv;
                } else { // r = -1
                    fnx = -rxv;
                    fny = -ryv;
                    fnz = -rzv;
                }

                double mag = std::sqrt(fnx * fnx + fny * fny + fnz * fnz);
                if (mag > 1e-30) {
                    fnx /= mag;
                    fny /= mag;
                    fnz /= mag;
                }

                mesh.nx(fid, k) = fnx;
                mesh.ny(fid, k) = fny;
                mesh.nz(fid, k) = fnz;
                mesh.sJ(fid, k) = mag * std::abs(mesh.J(nid, k));
                mesh.Fscale(fid, k) = mesh.sJ(fid, k) / std::abs(mesh.J(nid, k));
            }
        }
    }

    // Step 9: Node-level connectivity maps
    int totalSurfNodes = totalFaceNodes * mesh.K;
    mesh.vmapM.resize(totalSurfNodes);
    mesh.vmapP.resize(totalSurfNodes);

    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            for (int i = 0; i < Nfp; ++i) {
                int surfIdx = k * totalFaceNodes + f * Nfp + i;
                mesh.vmapM(surfIdx) = k * Np + basis.Fmask(i, f);
            }
        }
    }

    mesh.vmapP = mesh.vmapM; // default to self
    double matchTol = h * 1e-6;

    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            int k2 = mesh.EToE(k, f);
            int f2 = mesh.EToF(k, f);
            if (k2 == k && f2 == f)
                continue;

            for (int i = 0; i < Nfp; ++i) {
                int surfIdxM = k * totalFaceNodes + f * Nfp + i;
                int nM = basis.Fmask(i, f);
                double xM = mesh.x(nM, k), yM = mesh.y(nM, k), zM = mesh.z(nM, k);

                for (int j = 0; j < Nfp; ++j) {
                    int nP = basis.Fmask(j, f2);
                    double xP = mesh.x(nP, k2), yP = mesh.y(nP, k2), zP = mesh.z(nP, k2);
                    double d2 = (xM - xP) * (xM - xP) + (yM - yP) * (yM - yP) + (zM - zP) * (zM - zP);
                    if (d2 < matchTol * matchTol) {
                        mesh.vmapP(surfIdxM) = k2 * Np + nP;
                        break;
                    }
                }
            }
        }
    }

    // Step 10: Boundary detection and material assignment
    std::vector<int> boundaryIndices;
    for (int k = 0; k < mesh.K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            if (mesh.EToE(k, f) == k && mesh.EToF(k, f) == f) {
                // Find face centroid for material lookup
                Vec3d faceCentroid = Vec3d::Zero();
                for (int i = 0; i < Nfp; ++i) {
                    int nid = basis.Fmask(i, f);
                    faceCentroid += Vec3d(mesh.x(nid, k), mesh.y(nid, k), mesh.z(nid, k));
                }
                faceCentroid /= Nfp;

                Vec3f centF(static_cast<float>(faceCentroid.x()), static_cast<float>(faceCentroid.y()),
                            static_cast<float>(faceCentroid.z()));
                int wallIdx = findNearestWall(centF, walls, modelVertices);
                float avgAbs = 0.0f;
                for (float a : walls[wallIdx].absorption)
                    avgAbs += a;
                avgAbs /= NUM_FREQ_BANDS;
                double alpha = static_cast<double>(avgAbs);
                double Z = absorptionToImpedance(alpha);

                BoundaryFace3D bf;
                bf.element = k;
                bf.localFace = f;
                bf.impedance = Z;
                mesh.boundaryFaces.push_back(bf);

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

    mesh.boundaryImpedance = VecXd::Zero(totalSurfNodes);
    for (const auto& bf : mesh.boundaryFaces) {
        for (int i = 0; i < Nfp; ++i) {
            int surfIdx = bf.element * totalFaceNodes + bf.localFace * Nfp + i;
            mesh.boundaryImpedance(surfIdx) = bf.impedance;
        }
    }

    mesh.bcType = VecXi::Zero(totalSurfNodes);
    for (int i = 0; i < mesh.mapB.size(); ++i)
        mesh.bcType(mesh.mapB(i)) = 1;

    return mesh;
}

} // namespace dg
} // namespace prs
