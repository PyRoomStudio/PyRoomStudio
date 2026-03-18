#pragma once

#include "DGTypes.h"

namespace prs {
namespace dg {

// Precomputed operators for the 3D nodal DG reference tetrahedron.
// Reference element: vertices at (-1,-1,-1), (1,-1,-1), (-1,1,-1), (-1,-1,1)
struct Basis3D {
    int N;        // polynomial order
    int Np;       // (N+1)(N+2)(N+3)/6 nodes per element
    int Nfp;      // (N+1)(N+2)/2 nodes per face
    int Nfaces;   // 4

    VecXd r, s, t;       // node coordinates on reference element
    MatXd V, invV;       // Vandermonde and its inverse
    MatXd Dr, Ds, Dt;    // differentiation matrices
    MatXd LIFT;          // surface-to-volume lift operator
    MatXi Fmask;         // (Nfp x Nfaces) face node indices
};

Basis3D buildBasis3D(int N);

// Low-level building blocks
void rstToABC(const VecXd& r, const VecXd& s, const VecXd& t,
              VecXd& a, VecXd& b, VecXd& c);
MatXd vandermonde3D(int N, const VecXd& r, const VecXd& s, const VecXd& t);
void gradVandermonde3D(int N, const VecXd& r, const VecXd& s, const VecXd& t,
                       MatXd& V3Dr, MatXd& V3Ds, MatXd& V3Dt);
void warpAndBlendNodes3D(int N, VecXd& r, VecXd& s, VecXd& t);

} // namespace dg
} // namespace prs
