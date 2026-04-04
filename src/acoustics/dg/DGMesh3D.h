#pragma once

#include "acoustics/Bvh.h"
#include "core/Types.h"
#include "DGBasis3D.h"
#include "DGTypes.h"
#include "rendering/Viewport3D.h"

#include <vector>

namespace prs {
namespace dg {

struct Mesh3D {
    // Vertex coordinates
    VecXd VX, VY, VZ; // (Nv) vertex positions
    int Nv;           // number of vertices

    // Element connectivity: each row is a tetrahedron (4 vertex indices)
    MatXi EToV; // (K x 4)
    int K;      // number of elements

    // High-order node coordinates
    MatXd x, y, z; // (Np x K) physical coordinates

    // Geometric factors
    MatXd rx, ry, rz; // (Np x K)
    MatXd sx, sy, sz; // (Np x K)
    MatXd tx, ty, tz; // (Np x K)
    MatXd J;          // (Np x K) Jacobian determinant

    // Surface geometry
    MatXd nx, ny, nz; // (Nfp*Nfaces x K) outward face normals
    MatXd sJ;         // (Nfp*Nfaces x K) surface Jacobian
    MatXd Fscale;     // (Nfp*Nfaces x K)

    // Connectivity
    MatXi EToE;         // (K x 4) element-to-element neighbor
    MatXi EToF;         // (K x 4) element-to-face of neighbor
    VecXi vmapM, vmapP; // interior/exterior node maps
    VecXi mapB;         // boundary node indices
    VecXi bcType;       // BC type per surface node

    // Boundary data
    std::vector<BoundaryFace3D> boundaryFaces;
    VecXd boundaryImpedance;
};

// Build a 3D tetrahedral mesh from the room geometry.
// Uses a structured grid approach: fill bounding box with cubes split into tetrahedra,
// then classify interior/exterior using the BVH.
Mesh3D buildMesh3D(const std::vector<Viewport3D::WallInfo>& walls, const std::vector<Vec3f>& modelVertices,
                   const Bvh& bvh, const Basis3D& basis, double targetElementSize);

} // namespace dg
} // namespace prs
