#pragma once

#include "core/Types.h"
#include "DGBasis2D.h"
#include "DGTypes.h"
#include "rendering/Viewport3D.h"

#include <vector>

namespace prs {
namespace dg {

struct Mesh2D {
    // Vertex coordinates
    VecXd VX, VY; // (Nv) vertex positions
    int Nv;       // number of vertices

    // Element connectivity: each row is a triangle (3 vertex indices)
    MatXi EToV; // (K x 3)
    int K;      // number of elements

    // High-order node coordinates (mapped from reference element)
    MatXd x, y; // (Np x K) physical coordinates per node per element

    // Geometric factors
    MatXd rx, ry; // (Np x K) dr/dx, dr/dy
    MatXd sx, sy; // (Np x K) ds/dx, ds/dy
    MatXd J;      // (Np x K) Jacobian determinant

    // Surface geometry (for flux computation)
    MatXd nx, ny; // (Nfp*Nfaces x K) outward face normals
    MatXd sJ;     // (Nfp*Nfaces x K) surface Jacobian (edge length factor)
    MatXd Fscale; // (Nfp*Nfaces x K) = sJ / J(surface nodes)

    // Connectivity
    MatXi EToE;         // (K x 3) element-to-element neighbor (-1 = boundary)
    MatXi EToF;         // (K x 3) element-to-face of neighbor
    VecXi vmapM, vmapP; // interior/exterior node maps for flux computation
    VecXi mapB;         // boundary node indices (into vmapM)
    VecXi bcType;       // boundary condition type per boundary face (material index)

    // Boundary data
    std::vector<BoundaryEdge2D> boundaryEdges;
    VecXd boundaryImpedance; // impedance at each boundary surface node
};

struct RoomPolygon2D {
    std::vector<Vec2d> vertices;
    std::vector<double> edgeAbsorption;
    std::vector<double> edgeScattering;
};

// Extract a 2D polygon from 3D room geometry at a given height
RoomPolygon2D extractRoomPolygon(const std::vector<Viewport3D::WallInfo>& walls,
                                 const std::vector<Vec3f>& modelVertices, float sliceHeight);

// Build a 2D triangular mesh suitable for DG from the room polygon
Mesh2D buildMesh2D(const RoomPolygon2D& polygon, const Basis2D& basis, double targetElementSize);

} // namespace dg
} // namespace prs
