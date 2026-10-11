#include "DGBasis2D.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cassert>

namespace prs {
namespace dg {

// ---------- Jacobi polynomials ----------

double jacobiP(double x, double alpha, double beta, int n) {
    double g0 = std::pow(2.0, alpha + beta + 1.0)
               / (alpha + beta + 1.0)
               * std::tgamma(alpha + 1.0) * std::tgamma(beta + 1.0)
               / std::tgamma(alpha + beta + 1.0);
    if (n == 0) return 1.0 / std::sqrt(g0);

    if (n == 1) {
        double g1 = (alpha + 1.0) * (beta + 1.0) / (alpha + beta + 3.0) * g0;
        return ((alpha + beta + 2.0) * x / 2.0 + (alpha - beta) / 2.0) / std::sqrt(g1);
    }

    // Unnormalized DLMF three-term recurrence, normalized at the end.
    double Pprev = 1.0, Pcur = 0.5 * (alpha - beta + (alpha + beta + 2.0) * x);
    for (int i = 1; i < n; ++i) {
        double di = static_cast<double>(i);
        double c1 = 2.0 * (di + 1.0) * (di + alpha + beta + 1.0) * (2.0 * di + alpha + beta);
        double c2 = (2.0 * di + alpha + beta + 1.0) * (alpha * alpha - beta * beta);
        double c3 = (2.0 * di + alpha + beta) * (2.0 * di + alpha + beta + 1.0) * (2.0 * di + alpha + beta + 2.0);
        double c4 = 2.0 * (di + alpha) * (di + beta) * (2.0 * di + alpha + beta + 2.0);
        double Pnext = ((c2 + c3 * x) * Pcur - c4 * Pprev) / c1;
        Pprev = Pcur;
        Pcur = Pnext;
    }
    double dn = static_cast<double>(n);
    double hn = std::pow(2.0, alpha + beta + 1.0) / (2.0 * dn + alpha + beta + 1.0)
              * std::tgamma(dn + alpha + 1.0) * std::tgamma(dn + beta + 1.0)
              / (std::tgamma(dn + 1.0) * std::tgamma(dn + alpha + beta + 1.0));
    return Pcur / std::sqrt(hn);
}

double gradJacobiP(double x, double alpha, double beta, int n) {
    if (n == 0) return 0.0;
    return std::sqrt(n * (n + alpha + beta + 1.0))
         * jacobiP(x, alpha + 1.0, beta + 1.0, n - 1);
}

// Gauss-Lobatto quadrature nodes on [-1,1]
VecXd jacobiGL(double alpha, double beta, int n) {
    if (n == 1) {
        VecXd x(2);
        x(0) = -1.0;
        x(1) = 1.0;
        return x;
    }
    VecXd w;
    VecXd interior = jacobiGQ(alpha + 1.0, beta + 1.0, n - 2, w);
    VecXd x(n + 1);
    x(0) = -1.0;
    for (int i = 0; i < n - 1; ++i) x(i + 1) = interior(i);
    x(n) = 1.0;
    return x;
}

// Gauss quadrature nodes and weights via eigenvalue method
VecXd jacobiGQ(double alpha, double beta, int n, VecXd& weights) {
    if (n < 0) {
        weights.resize(1);
        weights(0) = std::pow(2.0, alpha + beta + 1.0)
                   * std::tgamma(alpha + 1.0) * std::tgamma(beta + 1.0)
                   / std::tgamma(alpha + beta + 2.0);
        VecXd x(1);
        x(0) = (alpha - beta) / (alpha + beta + 2.0);
        return x;
    }

    int np1 = n + 1;
    VecXd h1(np1);
    for (int i = 0; i < np1; ++i)
        h1(i) = 2.0 * i + alpha + beta;

    // Tridiagonal matrix (Golub-Welsch)
    VecXd mainDiag(np1);
    for (int i = 0; i < np1; ++i) {
        if (std::abs(h1(i) * (h1(i) + 2.0)) < 1e-30)
            mainDiag(i) = 0.0;
        else
            mainDiag(i) = -(alpha * alpha - beta * beta) / (h1(i) * (h1(i) + 2.0));
    }

    VecXd offDiag(np1 - 1);
    for (int i = 0; i < np1 - 1; ++i) {
        double di = static_cast<double>(i + 1);
        offDiag(i) = 2.0 / (h1(i) + 2.0)
                   * std::sqrt(di * (di + alpha + beta) * (di + alpha) * (di + beta)
                              / ((h1(i) + 1.0) * (h1(i) + 3.0)));
    }

    // Correct first entry if alpha+beta=0
    if (std::abs(alpha + beta) < 1e-14)
        mainDiag(0) = (alpha - beta) / (alpha + beta + 2.0);

    // Solve eigenvalue problem
    MatXd J = MatXd::Zero(np1, np1);
    for (int i = 0; i < np1; ++i) J(i, i) = mainDiag(i);
    for (int i = 0; i < np1 - 1; ++i) {
        J(i, i + 1) = offDiag(i);
        J(i + 1, i) = offDiag(i);
    }

    Eigen::SelfAdjointEigenSolver<MatXd> es(J);
    VecXd x = es.eigenvalues();
    MatXd V = es.eigenvectors();

    weights.resize(np1);
    double w0 = std::pow(2.0, alpha + beta + 1.0)
              * std::tgamma(alpha + 1.0) * std::tgamma(beta + 1.0)
              / std::tgamma(alpha + beta + 2.0);
    for (int i = 0; i < np1; ++i)
        weights(i) = w0 * V(0, i) * V(0, i);

    return x;
}

// ---------- Coordinate transforms ----------

void rsToAB(const VecXd& r, const VecXd& s, VecXd& a, VecXd& b) {
    int np = static_cast<int>(r.size());
    a.resize(np);
    b.resize(np);
    for (int i = 0; i < np; ++i) {
        if (std::abs(s(i) - 1.0) > 1e-14)
            a(i) = 2.0 * (1.0 + r(i)) / (1.0 - s(i)) - 1.0;
        else
            a(i) = -1.0;
        b(i) = s(i);
    }
}

// ---------- 2D orthonormal basis on reference triangle ----------

static double simplex2DP(const VecXd& a, const VecXd& b, int i, int j, int idx) {
    double h1 = jacobiP(a(idx), 0.0, 0.0, i);
    double h2 = jacobiP(b(idx), 2.0 * i + 1.0, 0.0, j);
    return std::sqrt(2.0) * h1 * h2 * std::pow(1.0 - b(idx), i);
}

MatXd vandermonde2D(int N, const VecXd& r, const VecXd& s) {
    int Np = (N + 1) * (N + 2) / 2;
    int npts = static_cast<int>(r.size());
    MatXd V2D(npts, Np);

    VecXd a, b;
    rsToAB(r, s, a, b);

    int sk = 0;
    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= N - i; ++j) {
            for (int n = 0; n < npts; ++n)
                V2D(n, sk) = simplex2DP(a, b, i, j, n);
            ++sk;
        }
    }
    return V2D;
}

void gradVandermonde2D(int N, const VecXd& r, const VecXd& s,
                       MatXd& V2Dr, MatXd& V2Ds) {
    int Np = (N + 1) * (N + 2) / 2;
    int npts = static_cast<int>(r.size());
    V2Dr.resize(npts, Np);
    V2Ds.resize(npts, Np);

    VecXd a, b;
    rsToAB(r, s, a, b);

    int sk = 0;
    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= N - i; ++j) {
            for (int n = 0; n < npts; ++n) {
                // Hesthaven & Warburton GradSimplex2DP: exact, and finite at the collapsed vertex s=1.
                double fa = jacobiP(a(n), 0.0, 0.0, i);
                double dfa = gradJacobiP(a(n), 0.0, 0.0, i);
                double gb = jacobiP(b(n), 2.0 * i + 1.0, 0.0, j);
                double dgb = gradJacobiP(b(n), 2.0 * i + 1.0, 0.0, j);
                double hb = 0.5 * (1.0 - b(n));
                double hbm1 = i > 0 ? std::pow(hb, i - 1) : 1.0;

                double dr = dfa * gb * hbm1;
                double tmp = dgb * std::pow(hb, i);
                if (i > 0) tmp -= 0.5 * i * gb * hbm1;
                double ds = 0.5 * (1.0 + a(n)) * dr + fa * tmp;

                double scale = std::pow(2.0, i + 0.5);
                V2Dr(n, sk) = scale * dr;
                V2Ds(n, sk) = scale * ds;
            }
            ++sk;
        }
    }
}

// ---------- Warp & Blend nodes on equilateral triangle ----------

static VecXd warpFactor(int N, const VecXd& rout) {
    VecXd LGLr = jacobiGL(0.0, 0.0, N);

    // Equidistant nodes on [-1,1]
    int Np1 = N + 1;
    VecXd req(Np1);
    for (int i = 0; i < Np1; ++i)
        req(i) = -1.0 + 2.0 * i / static_cast<double>(N);

    // Build Vandermonde based on equidistant nodes
    MatXd Veq(Np1, Np1);
    for (int i = 0; i < Np1; ++i)
        for (int j = 0; j < Np1; ++j)
            Veq(i, j) = jacobiP(req(i), 0.0, 0.0, j);

    int nr = static_cast<int>(rout.size());
    MatXd Pmat(nr, Np1);
    for (int i = 0; i < nr; ++i)
        for (int j = 0; j < Np1; ++j)
            Pmat(i, j) = jacobiP(rout(i), 0.0, 0.0, j);

    // Unscaled warp; the (1-r^2) edge blending is applied by the caller.
    return Pmat * Veq.inverse().transpose() * (LGLr - req);
}

void warpAndBlendNodes2D(int N, VecXd& r, VecXd& s) {
    int Np = (N + 1) * (N + 2) / 2;
    r.resize(Np);
    s.resize(Np);

    // Start with equidistant nodes on equilateral triangle
    // Vertices: (-1,-1/sqrt(3)), (1,-1/sqrt(3)), (0,2/sqrt(3))
    // But we use the reference triangle with vertices (-1,-1), (1,-1), (-1,1)
    // Equidistant nodes in barycentric coordinates
    VecXd L1(Np), L2(Np), L3(Np);
    int sk = 0;
    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= N - i; ++j) {
            L1(sk) = static_cast<double>(i) / N;
            L3(sk) = static_cast<double>(j) / N;
            L2(sk) = 1.0 - L1(sk) - L3(sk);
            ++sk;
        }
    }

    // Convert barycentric (L1,L2,L3) to (r,s) on reference triangle [-1,1]^2
    for (int i = 0; i < Np; ++i) {
        r(i) = -L2(i) + L3(i) - L1(i);
        s(i) = -L2(i) - L3(i) + L1(i);
    }

    // For N <= 1, no warp needed
    if (N <= 1) return;

    // Blending functions (Hesthaven & Warburton eq 10.3), and each edge's parameter
    VecXd blend1(Np), blend2(Np), blend3(Np);
    VecXd e1param(Np), e2param(Np), e3param(Np);
    for (int i = 0; i < Np; ++i) {
        blend1(i) = 4.0 * L2(i) * L3(i);
        blend2(i) = 4.0 * L1(i) * L3(i);
        blend3(i) = 4.0 * L1(i) * L2(i);
        e1param(i) = L3(i) - L2(i);
        e2param(i) = L1(i) - L3(i);
        e3param(i) = L2(i) - L1(i);
    }

    VecXd warp1 = warpFactor(N, e1param);
    VecXd warp2 = warpFactor(N, e2param);
    VecXd warp3 = warpFactor(N, e3param);

    // Equilateral triangle vertices
    double v1x = -1.0, v1y = -1.0 / std::sqrt(3.0);
    double v2x = 1.0,  v2y = -1.0 / std::sqrt(3.0);
    double v3x = 0.0,  v3y = 2.0 / std::sqrt(3.0);

    // Equidistant (x,y) on equilateral triangle
    VecXd xeq(Np), yeq(Np);
    for (int i = 0; i < Np; ++i) {
        xeq(i) = L1(i) * v1x + L2(i) * v2x + L3(i) * v3x;
        yeq(i) = L1(i) * v1y + L2(i) * v2y + L3(i) * v3y;
    }

    // Tangent vectors for each edge of equilateral triangle
    double t1x = v3x - v2x, t1y = v3y - v2y;
    double t2x = v1x - v3x, t2y = v1y - v3y;
    double t3x = v2x - v1x, t3y = v2y - v1y;
    double l1 = std::sqrt(t1x * t1x + t1y * t1y);
    double l2 = std::sqrt(t2x * t2x + t2y * t2y);
    double l3 = std::sqrt(t3x * t3x + t3y * t3y);
    t1x /= l1; t1y /= l1;
    t2x /= l2; t2y /= l2;
    t3x /= l3; t3y /= l3;

    VecXd xw(Np), yw(Np);
    for (int i = 0; i < Np; ++i) {
        xw(i) = xeq(i) + blend1(i) * warp1(i) * t1x
                        + blend2(i) * warp2(i) * t2x
                        + blend3(i) * warp3(i) * t3x;
        yw(i) = yeq(i) + blend1(i) * warp1(i) * t1y
                        + blend2(i) * warp2(i) * t2y
                        + blend3(i) * warp3(i) * t3y;
    }

    // Convert (x,y) on equilateral back to (r,s) on reference [-1,1] triangle
    // Reference triangle vertices: (-1,-1), (1,-1), (-1,1)
    // Equilateral vertices: v1, v2, v3 map to ref: (-1,-1), (1,-1), (-1,1)
    // Inverse affine transform:
    // x = L1*v1x + L2*v2x + L3*v3x, L1+L2+L3=1
    // r = -L1 + L2 - L3 = -1 + 2*L2 - 2*L3 ... hmm
    // Actually: r = 2*L2 - 1, s = 2*L3 - 1 (for standard ref triangle)
    // So L2 = (1+r)/2, L3 = (1+s)/2, L1 = 1 - L2 - L3

    // From (x,y) to (L1,L2,L3) via inverse of affine:
    // [x] = [v1x v2x v3x] [L1]
    // [y]   [v1y v2y v3y] [L2]
    // 1   = [1   1   1  ] [L3]
    MatXd Tmat(3, 3);
    Tmat << v1x, v2x, v3x,
            v1y, v2y, v3y,
            1.0, 1.0, 1.0;
    MatXd Tinv = Tmat.inverse();

    for (int i = 0; i < Np; ++i) {
        Vec3d rhs(xw(i), yw(i), 1.0);
        Vec3d bary = Tinv * rhs;
        // bary = (L1, L2, L3)
        r(i) = -bary(0) + bary(1) - bary(2);
        s(i) = -bary(0) - bary(1) + bary(2);
    }
}

// ---------- Build complete 2D basis ----------

Basis2D buildBasis2D(int N) {
    Basis2D B;
    B.N = N;
    B.Np = (N + 1) * (N + 2) / 2;
    B.Nfp = N + 1;
    B.Nfaces = 3;

    // Compute nodes
    warpAndBlendNodes2D(N, B.r, B.s);

    // Vandermonde
    B.V = vandermonde2D(N, B.r, B.s);
    B.invV = B.V.inverse();

    // Differentiation matrices
    MatXd V2Dr, V2Ds;
    gradVandermonde2D(N, B.r, B.s, V2Dr, V2Ds);
    B.Dr = V2Dr * B.invV;
    B.Ds = V2Ds * B.invV;

    // Build face masks: identify nodes on each face
    // Face 1: s = -1
    // Face 2: r + s = 0 (hypotenuse)
    // Face 3: r = -1
    B.Fmask.resize(B.Nfp, B.Nfaces);
    B.Fmask.setConstant(-1);

    double tol = 1e-7;
    std::vector<int> f1, f2, f3;
    for (int i = 0; i < B.Np; ++i) {
        if (std::abs(B.s(i) + 1.0) < tol) f1.push_back(i);
        if (std::abs(B.r(i) + B.s(i)) < tol) f2.push_back(i);
        if (std::abs(B.r(i) + 1.0) < tol) f3.push_back(i);
    }

    // Sort face nodes by coordinate along each edge
    std::sort(f1.begin(), f1.end(), [&](int a, int b) { return B.r(a) < B.r(b); });
    std::sort(f2.begin(), f2.end(), [&](int a, int b) { return B.r(a) > B.r(b); });
    std::sort(f3.begin(), f3.end(), [&](int a, int b) { return B.s(a) > B.s(b); });

    for (int i = 0; i < B.Nfp && i < static_cast<int>(f1.size()); ++i) B.Fmask(i, 0) = f1[i];
    for (int i = 0; i < B.Nfp && i < static_cast<int>(f2.size()); ++i) B.Fmask(i, 1) = f2[i];
    for (int i = 0; i < B.Nfp && i < static_cast<int>(f3.size()); ++i) B.Fmask(i, 2) = f3[i];

    // LIFT matrix: LIFT = V * V^T * Emat, where Emat maps face to volume
    // Emat is (Np x Nfaces*Nfp) with entries for face integration
    int totalFaceNodes = B.Nfaces * B.Nfp;
    MatXd Emat = MatXd::Zero(B.Np, totalFaceNodes);

    // For each face, compute the face mass matrix and embed into Emat
    for (int f = 0; f < B.Nfaces; ++f) {
        // Extract face node coordinates (1D)
        VecXd faceR(B.Nfp);
        for (int i = 0; i < B.Nfp; ++i) {
            int idx = B.Fmask(i, f);
            if (f == 0)
                faceR(i) = B.r(idx); // parameterize along r for face 1
            else if (f == 1)
                faceR(i) = B.r(idx); // along r for hypotenuse
            else
                faceR(i) = B.s(idx); // along s for face 3
        }

        // 1D Vandermonde on face
        MatXd Vface(B.Nfp, B.Nfp);
        for (int i = 0; i < B.Nfp; ++i)
            for (int j = 0; j < B.Nfp; ++j)
                Vface(i, j) = jacobiP(faceR(i), 0.0, 0.0, j);

        // Face mass matrix: Mface = inv(Vface * Vface^T)
        MatXd Mface = (Vface * Vface.transpose()).inverse();

        // Insert into Emat: Emat(Fmask(:,f), f*Nfp:(f+1)*Nfp) = Mface
        for (int i = 0; i < B.Nfp; ++i) {
            for (int j = 0; j < B.Nfp; ++j) {
                Emat(B.Fmask(i, f), f * B.Nfp + j) = Mface(i, j);
            }
        }
    }

    // LIFT = V * (V^T * Emat)
    B.LIFT = B.V * (B.V.transpose() * Emat);

    return B;
}

} // namespace dg
} // namespace prs
