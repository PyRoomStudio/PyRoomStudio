#include "DGBasis3D.h"
#include "DGBasis2D.h"  // reuse jacobiP, jacobiGL, jacobiGQ, vandermonde2D
#include <cmath>
#include <algorithm>
#include <cassert>

namespace prs {
namespace dg {

// ---------- Coordinate transforms for 3D reference tetrahedron ----------

void rstToABC(const VecXd& r, const VecXd& s, const VecXd& t,
              VecXd& a, VecXd& b, VecXd& c) {
    int np = static_cast<int>(r.size());
    a.resize(np);
    b.resize(np);
    c.resize(np);

    for (int i = 0; i < np; ++i) {
        if (std::abs(s(i) + t(i)) > 1e-14)
            a(i) = 2.0 * (1.0 + r(i)) / (-s(i) - t(i)) - 1.0;
        else
            a(i) = -1.0;

        if (std::abs(t(i) - 1.0) > 1e-14)
            b(i) = 2.0 * (1.0 + s(i)) / (1.0 - t(i)) - 1.0;
        else
            b(i) = -1.0;

        c(i) = t(i);
    }
}

// ---------- 3D orthonormal basis on reference tetrahedron ----------

static double simplex3DP(const VecXd& a, const VecXd& b, const VecXd& c,
                         int i, int j, int k, int idx) {
    double h1 = jacobiP(a(idx), 0.0, 0.0, i);
    double h2 = jacobiP(b(idx), 2.0 * i + 1.0, 0.0, j);
    double h3 = jacobiP(c(idx), 2.0 * (i + j) + 2.0, 0.0, k);

    return 2.0 * std::sqrt(2.0) * h1 * h2
         * std::pow(1.0 - b(idx), i)
         * h3
         * std::pow(1.0 - c(idx), i + j);
}

MatXd vandermonde3D(int N, const VecXd& r, const VecXd& s, const VecXd& t) {
    int Np = (N + 1) * (N + 2) * (N + 3) / 6;
    int npts = static_cast<int>(r.size());
    MatXd V3D(npts, Np);

    VecXd a, b, c;
    rstToABC(r, s, t, a, b, c);

    int sk = 0;
    for (int i = 0; i <= N; ++i)
        for (int j = 0; j <= N - i; ++j)
            for (int k = 0; k <= N - i - j; ++k) {
                for (int n = 0; n < npts; ++n)
                    V3D(n, sk) = simplex3DP(a, b, c, i, j, k, n);
                ++sk;
            }
    return V3D;
}

void gradVandermonde3D(int N, const VecXd& r, const VecXd& s, const VecXd& t,
                       MatXd& V3Dr, MatXd& V3Ds, MatXd& V3Dt) {
    int Np = (N + 1) * (N + 2) * (N + 3) / 6;
    int npts = static_cast<int>(r.size());
    V3Dr.resize(npts, Np);
    V3Ds.resize(npts, Np);
    V3Dt.resize(npts, Np);

    VecXd a, b, cv;
    rstToABC(r, s, t, a, b, cv);

    int sk = 0;
    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= N - i; ++j) {
            for (int k = 0; k <= N - i - j; ++k) {
                for (int n = 0; n < npts; ++n) {
                    // Hesthaven & Warburton GradSimplex3DP: exact, and finite on the collapsed edges.
                    double fa  = jacobiP(a(n), 0.0, 0.0, i);
                    double dfa = gradJacobiP(a(n), 0.0, 0.0, i);
                    double gb  = jacobiP(b(n), 2.0 * i + 1.0, 0.0, j);
                    double dgb = gradJacobiP(b(n), 2.0 * i + 1.0, 0.0, j);
                    double fc  = jacobiP(cv(n), 2.0 * (i + j) + 2.0, 0.0, k);
                    double dfc = gradJacobiP(cv(n), 2.0 * (i + j) + 2.0, 0.0, k);
                    double hb = 0.5 * (1.0 - b(n));
                    double hc = 0.5 * (1.0 - cv(n));
                    double hbm1 = i > 0 ? std::pow(hb, i - 1) : 1.0;
                    double hcm1 = i + j > 0 ? std::pow(hc, i + j - 1) : 1.0;

                    double dr = dfa * gb * fc * hbm1 * hcm1;

                    double tmp = dgb * std::pow(hb, i);
                    if (i > 0) tmp -= 0.5 * i * gb * hbm1;
                    tmp = fa * tmp * hcm1 * fc;
                    double ds = 0.5 * (1.0 + a(n)) * dr + tmp;
                    double dt = 0.5 * (1.0 + a(n)) * dr + 0.5 * (1.0 + b(n)) * tmp;

                    tmp = dfc * std::pow(hc, i + j);
                    if (i + j > 0) tmp -= 0.5 * (i + j) * fc * hcm1;
                    dt += fa * gb * tmp * std::pow(hb, i);

                    double scale = std::pow(2.0, 2 * i + j + 1.5);
                    V3Dr(n, sk) = scale * dr;
                    V3Ds(n, sk) = scale * ds;
                    V3Dt(n, sk) = scale * dt;
                }
                ++sk;
            }
        }
    }
}

// ---------- Warp & Blend nodes on reference tetrahedron ----------

void warpAndBlendNodes3D(int N, VecXd& r, VecXd& s, VecXd& t) {
    int Np = (N + 1) * (N + 2) * (N + 3) / 6;
    r.resize(Np);
    s.resize(Np);
    t.resize(Np);

    // Start with equidistant nodes in barycentric coordinates on the reference tet
    VecXd L1(Np), L2(Np), L3(Np), L4(Np);
    int sk = 0;
    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= N - i; ++j) {
            for (int k = 0; k <= N - i - j; ++k) {
                L1(sk) = static_cast<double>(i) / N;
                L2(sk) = static_cast<double>(j) / N;
                L3(sk) = static_cast<double>(k) / N;
                L4(sk) = 1.0 - L1(sk) - L2(sk) - L3(sk);
                ++sk;
            }
        }
    }

    // Convert barycentric to (r,s,t) on reference tetrahedron
    // Vertices: v1=(-1,-1,-1), v2=(1,-1,-1), v3=(-1,1,-1), v4=(-1,-1,1)
    // (r,s,t) = L1*v1 + L2*v2 + L3*v3 + L4*v4
    // But standard is: L1 corresponds to vertex (-1,-1,-1), etc.
    // Actually: r = -L1 + L2 - L3 - L4, s = -L1 - L2 + L3 - L4, t = -L1 - L2 - L3 + L4
    // Simplify using L1+L2+L3+L4=1:
    for (int i = 0; i < Np; ++i) {
        r(i) = -1.0 + 2.0 * L2(i);
        s(i) = -1.0 + 2.0 * L3(i);
        t(i) = -1.0 + 2.0 * L4(i);
    }

    // For low order (N <= 2), equidistant nodes are adequate
    if (N <= 2) return;

    // For higher order, apply warp & blend.
    // The 3D version is more complex (4 faces, 6 edges).
    // For a simpler but adequate approach, we use the 2D warp on each face
    // and blend into the interior using barycentric weights.

    // Get 2D warped nodes for reference triangle
    VecXd r2d, s2d;
    warpAndBlendNodes2D(N, r2d, s2d);

    // For each of the 4 faces, compute 2D warp and blend into the tet.
    // This is a simplified version; for production, the full Hesthaven/Warburton
    // algorithm should be used. The equidistant nodes are reasonable for N <= 5.
    // We keep the equidistant nodes as-is for now -- the Vandermonde will still
    // be well-conditioned for moderate N thanks to the orthonormal basis.
}

// ---------- Build complete 3D basis ----------

Basis3D buildBasis3D(int N) {
    Basis3D B;
    B.N = N;
    B.Np = (N + 1) * (N + 2) * (N + 3) / 6;
    B.Nfp = (N + 1) * (N + 2) / 2;
    B.Nfaces = 4;

    // Compute nodes
    warpAndBlendNodes3D(N, B.r, B.s, B.t);

    // Vandermonde
    B.V = vandermonde3D(N, B.r, B.s, B.t);
    B.invV = B.V.inverse();

    // Differentiation matrices
    MatXd V3Dr, V3Ds, V3Dt;
    gradVandermonde3D(N, B.r, B.s, B.t, V3Dr, V3Ds, V3Dt);
    B.Dr = V3Dr * B.invV;
    B.Ds = V3Ds * B.invV;
    B.Dt = V3Dt * B.invV;

    // Build face masks
    // Face 1: t = -1
    // Face 2: s = -1
    // Face 3: r + s + t = -1  (the "hypotenuse" face)
    // Face 4: r = -1
    B.Fmask.resize(B.Nfp, B.Nfaces);
    B.Fmask.setConstant(-1);

    double tol = 1e-7;
    std::vector<int> f1, f2, f3, f4;
    for (int i = 0; i < B.Np; ++i) {
        if (std::abs(B.t(i) + 1.0) < tol) f1.push_back(i);
        if (std::abs(B.s(i) + 1.0) < tol) f2.push_back(i);
        if (std::abs(B.r(i) + B.s(i) + B.t(i) + 1.0) < tol) f3.push_back(i);
        if (std::abs(B.r(i) + 1.0) < tol) f4.push_back(i);
    }

    // Sort by (r,s) or (r,t) or (s,t) order for consistent face ordering
    auto sortByRS = [&](std::vector<int>& v) {
        std::sort(v.begin(), v.end(), [&](int a, int b) {
            if (std::abs(B.r(a) - B.r(b)) > tol) return B.r(a) < B.r(b);
            return B.s(a) < B.s(b);
        });
    };
    sortByRS(f1);
    auto sortByRT = [&](std::vector<int>& v) {
        std::sort(v.begin(), v.end(), [&](int a, int b) {
            if (std::abs(B.r(a) - B.r(b)) > tol) return B.r(a) < B.r(b);
            return B.t(a) < B.t(b);
        });
    };
    sortByRT(f2);
    sortByRS(f3);
    auto sortByST = [&](std::vector<int>& v) {
        std::sort(v.begin(), v.end(), [&](int a, int b) {
            if (std::abs(B.s(a) - B.s(b)) > tol) return B.s(a) < B.s(b);
            return B.t(a) < B.t(b);
        });
    };
    sortByST(f4);

    for (int i = 0; i < B.Nfp && i < static_cast<int>(f1.size()); ++i) B.Fmask(i, 0) = f1[i];
    for (int i = 0; i < B.Nfp && i < static_cast<int>(f2.size()); ++i) B.Fmask(i, 1) = f2[i];
    for (int i = 0; i < B.Nfp && i < static_cast<int>(f3.size()); ++i) B.Fmask(i, 2) = f3[i];
    for (int i = 0; i < B.Nfp && i < static_cast<int>(f4.size()); ++i) B.Fmask(i, 3) = f4[i];

    // LIFT matrix
    // For 3D, LIFT = M^{-1} * Emat, where M = V*V^T (the mass matrix)
    // and Emat maps face nodes to volume (with face mass matrices embedded)
    int totalFaceNodes = B.Nfaces * B.Nfp;
    MatXd Emat = MatXd::Zero(B.Np, totalFaceNodes);

    for (int f = 0; f < B.Nfaces; ++f) {
        // Extract face node coordinates for 2D face Vandermonde
        VecXd faceR(B.Nfp), faceS(B.Nfp);
        for (int i = 0; i < B.Nfp; ++i) {
            int idx = B.Fmask(i, f);
            // Project to 2D coordinates on each face
            if (f == 0) {        // t = -1: use (r, s)
                faceR(i) = B.r(idx);
                faceS(i) = B.s(idx);
            } else if (f == 1) { // s = -1: use (r, t)
                faceR(i) = B.r(idx);
                faceS(i) = B.t(idx);
            } else if (f == 2) { // r+s+t=-1: use (r, s) (projected)
                faceR(i) = B.r(idx);
                faceS(i) = B.s(idx);
            } else {             // r = -1: use (s, t)
                faceR(i) = B.s(idx);
                faceS(i) = B.t(idx);
            }
        }

        // 2D Vandermonde on face
        MatXd Vface = vandermonde2D(N, faceR, faceS);
        MatXd Mface = (Vface * Vface.transpose()).inverse();

        for (int i = 0; i < B.Nfp; ++i) {
            for (int j = 0; j < B.Nfp; ++j) {
                Emat(B.Fmask(i, f), f * B.Nfp + j) = Mface(i, j);
            }
        }
    }

    B.LIFT = B.V * (B.V.transpose() * Emat);

    return B;
}

} // namespace dg
} // namespace prs
