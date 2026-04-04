#pragma once

#include "DGTypes.h"

namespace prs {
namespace dg {

// Precomputed operators for the 2D nodal DG reference triangle.
// Reference element: equilateral triangle mapped to (r,s) in [-1,1]
// using collapsed coordinates (Hesthaven & Warburton, Ch. 10).
struct Basis2D {
    int N;         // polynomial order
    int Np;        // (N+1)(N+2)/2 nodes per element
    int Nfp;       // N+1 nodes per face
    int Nfaces;    // 3

    VecXd r, s;    // node coordinates on reference element
    MatXd V, invV; // Vandermonde and its inverse
    MatXd Dr, Ds;  // differentiation matrices
    MatXd LIFT;    // surface-to-volume lift operator
    MatXi Fmask;   // (Nfp x Nfaces) face node indices

    VecXd Fscale;  // not used on reference; filled per-element later
};

Basis2D buildBasis2D(int N);

// Low-level building blocks (exposed for testing / 3D reuse)
double jacobiP(double x, double alpha, double beta, int n);
double gradJacobiP(double x, double alpha, double beta, int n);
VecXd jacobiGL(double alpha, double beta, int n);
VecXd jacobiGQ(double alpha, double beta, int n, VecXd& weights);

void rsToAB(const VecXd& r, const VecXd& s, VecXd& a, VecXd& b);
MatXd vandermonde2D(int N, const VecXd& r, const VecXd& s);
void gradVandermonde2D(int N, const VecXd& r, const VecXd& s, MatXd& V2Dr, MatXd& V2Ds);
void warpAndBlendNodes2D(int N, VecXd& r, VecXd& s);

} // namespace dg
} // namespace prs
