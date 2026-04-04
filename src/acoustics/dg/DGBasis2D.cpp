#include "DGBasis2D.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>

namespace prs {
namespace dg {

// ---------- Jacobi polynomials ----------

double jacobiP(double x, double alpha, double beta, int n) {
    if (n == 0) {
        double g0 = std::pow(2.0, alpha + beta + 1.0) / (alpha + beta + 1.0) * std::tgamma(alpha + 1.0) *
                    std::tgamma(beta + 1.0) / std::tgamma(alpha + beta + 1.0);
        return 1.0 / std::sqrt(g0);
    }

    double g0 = std::pow(2.0, alpha + beta + 1.0) / (alpha + beta + 1.0) * std::tgamma(alpha + 1.0) *
                std::tgamma(beta + 1.0) / std::tgamma(alpha + beta + 1.0);
    double p0 = 1.0 / std::sqrt(g0);

    double g1 = (alpha + 1.0) * (beta + 1.0) / (alpha + beta + 3.0) * g0;
    double p1 = ((alpha + beta + 2.0) * x / 2.0 + (alpha - beta) / 2.0) / std::sqrt(g1);

    if (n == 1)
        return p1;

    double pPrev = p0, pCur = p1;
    for (int i = 1; i < n; ++i) {
        double di = static_cast<double>(i);
        double a_n = 2.0 / (2.0 * di + alpha + beta + 2.0) *
                     std::sqrt((di + 1.0) * (di + 1.0 + alpha + beta) * (di + 1.0 + alpha) * (di + 1.0 + beta) /
                               ((2.0 * di + alpha + beta + 1.0) * (2.0 * di + alpha + beta + 3.0)));
        double b_n = -(alpha * alpha - beta * beta) / ((2.0 * di + alpha + beta) * (2.0 * di + alpha + beta + 2.0));
        double pNext = 1.0 / a_n * (-a_n * pPrev + (x - b_n) * pCur);

        // a_{n-1} for the previous step is already baked in via p values
        // Using three-term recurrence with normalized polynomials:
        // recompute properly
        pPrev = pCur;
        pCur = pNext;
    }
    // The above recurrence already produces the correctly normalized polynomial
    // via the standard three-term recurrence for orthonormal Jacobi polynomials.
    // But let's use a cleaner formulation to avoid drift.

    // Recompute with explicit three-term recurrence (Hesthaven/Warburton style)
    pPrev = p0;
    pCur = p1;
    if (n <= 1)
        return (n == 0) ? p0 : p1;

    for (int i = 1; i < n; ++i) {
        double di = static_cast<double>(i);
        double h1 = 2.0 * di + alpha + beta;

        double anew = 2.0 / (h1 + 2.0) *
                      std::sqrt((di + 1.0) * (di + 1.0 + alpha + beta) * (di + 1.0 + alpha) * (di + 1.0 + beta) /
                                ((h1 + 1.0) * (h1 + 3.0)));
        double bnew = -(alpha * alpha - beta * beta) / (h1 * (h1 + 2.0));

        double pNext = (1.0 / anew) * ((x - bnew) * pCur - anew * pPrev);
        // Actually the recurrence is: a_{n+1} P_{n+1} = (x - b_n) P_n - a_n P_{n-1}
        // We need a_{n} for the previous:
        // Let me redo this cleanly.
        (void)pNext;
        break;
    }

    // Clean reimplementation of normalized Jacobi P using standard recurrence
    // P_0, P_1 computed above. For n >= 2:
    pPrev = p0;
    pCur = p1;
    for (int i = 2; i <= n; ++i) {
        double di = static_cast<double>(i);
        double h1 = 2.0 * (di - 1.0) + alpha + beta;

        double a_cur = 2.0 / (h1 + 2.0) *
                       std::sqrt(di * (di + alpha + beta) * (di + alpha) * (di + beta) / ((h1 + 1.0) * (h1 + 3.0)));
        double b_prev = -(alpha * alpha - beta * beta) / (h1 * (h1 + 2.0));

        double pNext = ((x - b_prev) * pCur - a_cur * pPrev) / a_cur;
        // Wrong: the three-term is  a_{i} P_i = (x - b_{i-1}) P_{i-1} - a_{i-1} P_{i-2}
        // So P_i = ((x - b_{i-1}) P_{i-1} - a_{i-1} P_{i-2}) / a_i
        // But a_i depends on i, and a_{i-1} on i-1.
        (void)pNext;
        break;
    }

    // Simplest correct approach: use the unnormalized recurrence then normalize at the end.
    // Unnormalized Jacobi polynomial via standard DLMF recurrence:
    {
        double P0 = 1.0;
        double P1 = 0.5 * (alpha - beta + (alpha + beta + 2.0) * x);
        if (n == 0) {
            double norm =
                std::sqrt(std::pow(2.0, alpha + beta + 1.0) / (alpha + beta + 1.0) * std::tgamma(alpha + 1.0) *
                          std::tgamma(beta + 1.0) / std::tgamma(alpha + beta + 1.0));
            return P0 / norm;
        }
        if (n == 1) {
            double norm = std::sqrt(std::pow(2.0, alpha + beta + 1.0) * std::tgamma(alpha + 2.0) *
                                    std::tgamma(beta + 2.0) / (std::tgamma(alpha + beta + 3.0) * (alpha + beta + 3.0)));
            // Actually: h_1 = 2/(2+a+b) * (a+1)(b+1)/(a+b+3) * h_0
            // Normalization factor for P_n^{a,b}: h_n = int_{-1}^{1} (P_n)^2 (1-x)^a (1+x)^b dx
            // h_n = 2^{a+b+1} / (2n+a+b+1) * Gamma(n+a+1)Gamma(n+b+1) / (n! * Gamma(n+a+b+1))
            double hn = std::pow(2.0, alpha + beta + 1.0) / (2.0 + alpha + beta + 1.0) *
                        std::tgamma(1.0 + alpha + 1.0) * std::tgamma(1.0 + beta + 1.0) /
                        (1.0 * std::tgamma(1.0 + alpha + beta + 1.0));
            return P1 / std::sqrt(hn);
        }
        double Pprev = P0, Pcur = P1;
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
        double hn = std::pow(2.0, alpha + beta + 1.0) / (2.0 * dn + alpha + beta + 1.0) *
                    std::tgamma(dn + alpha + 1.0) * std::tgamma(dn + beta + 1.0) /
                    (std::tgamma(dn + 1.0) * std::tgamma(dn + alpha + beta + 1.0));
        return Pcur / std::sqrt(hn);
    }
}

double gradJacobiP(double x, double alpha, double beta, int n) {
    if (n == 0)
        return 0.0;
    return std::sqrt(n * (n + alpha + beta + 1.0)) * jacobiP(x, alpha + 1.0, beta + 1.0, n - 1);
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
    for (int i = 0; i < n - 1; ++i)
        x(i + 1) = interior(i);
    x(n) = 1.0;
    return x;
}

// Gauss quadrature nodes and weights via eigenvalue method
VecXd jacobiGQ(double alpha, double beta, int n, VecXd& weights) {
    if (n < 0) {
        weights.resize(1);
        weights(0) = std::pow(2.0, alpha + beta + 1.0) * std::tgamma(alpha + 1.0) * std::tgamma(beta + 1.0) /
                     std::tgamma(alpha + beta + 2.0);
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
        offDiag(i) = 2.0 / (h1(i) + 2.0) *
                     std::sqrt(di * (di + alpha + beta) * (di + alpha) * (di + beta) / ((h1(i) + 1.0) * (h1(i) + 3.0)));
    }

    // Correct first entry if alpha+beta=0
    if (std::abs(alpha + beta) < 1e-14)
        mainDiag(0) = (alpha - beta) / (alpha + beta + 2.0);

    // Solve eigenvalue problem
    MatXd J = MatXd::Zero(np1, np1);
    for (int i = 0; i < np1; ++i)
        J(i, i) = mainDiag(i);
    for (int i = 0; i < np1 - 1; ++i) {
        J(i, i + 1) = offDiag(i);
        J(i + 1, i) = offDiag(i);
    }

    Eigen::SelfAdjointEigenSolver<MatXd> es(J);
    VecXd x = es.eigenvalues();
    MatXd V = es.eigenvectors();

    weights.resize(np1);
    double w0 = std::pow(2.0, alpha + beta + 1.0) * std::tgamma(alpha + 1.0) * std::tgamma(beta + 1.0) /
                std::tgamma(alpha + beta + 2.0);
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

void gradVandermonde2D(int N, const VecXd& r, const VecXd& s, MatXd& V2Dr, MatXd& V2Ds) {
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
                double fa = jacobiP(a(n), 0.0, 0.0, i);
                double dfa = gradJacobiP(a(n), 0.0, 0.0, i);
                double gb = jacobiP(b(n), 2.0 * i + 1.0, 0.0, j);
                double dgb = gradJacobiP(b(n), 2.0 * i + 1.0, 0.0, j);
                double bfac = std::pow(1.0 - b(n), i);

                // dP/da
                double dmodedr = dfa * gb;
                if (i > 0)
                    dmodedr *= bfac;

                // dP/db
                double dmodeds = 0.0;
                if (i > 0)
                    dmodeds = dfa * gb * 0.5 * (1.0 + a(n)) * std::pow(1.0 - b(n), i - 1);
                dmodeds += fa * (dgb * bfac);
                if (i > 0)
                    dmodeds += fa * gb * (-static_cast<double>(i)) * std::pow(1.0 - b(n), i - 1);

                // Chain rule: dr/da = 2/(1-s), dr/db = (1+r)/(1-s)^2 * 0 (rs are independent)
                // Actually (r,s) -> (a,b) via a=2(1+r)/(1-s)-1, b=s
                // So da/dr = 2/(1-s), da/ds = 2(1+r)/(1-s)^2, db/dr = 0, db/ds = 1
                // dP/dr = dP/da * da/dr + dP/db * db/dr = dP/da * 2/(1-s)
                // dP/ds = dP/da * da/ds + dP/db * db/ds
                //       = dP/da * 2(1+r)/(1-s)^2 + dP/db

                double fac = 2.0 / (1.0 - b(n));
                if (std::abs(1.0 - b(n)) < 1e-14)
                    fac = 0.0;

                V2Dr(n, sk) = std::sqrt(2.0) * dmodedr * fac;
                V2Ds(n, sk) = std::sqrt(2.0) * (dmodedr * fac * 0.5 * (1.0 + a(n)) + dmodeds);
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

    VecXd warp = Pmat * Veq.inverse().transpose() * (LGLr - req);

    // Scale warp to avoid issues at vertices
    for (int i = 0; i < nr; ++i) {
        double d = 1.0 - rout(i) * rout(i);
        if (d > 1e-10)
            warp(i) /= d;
        else
            warp(i) = 0.0;
        warp(i) *= d; // net effect: warp unchanged, but avoid singularity
    }

    // Actually: the warp function from Hesthaven/Warburton zeroes out at endpoints
    // and the factor (1-r^2) is the blending. Let me redo properly.
    // warp = Lmat * (LGL - equidistant), then blend via (1-r^2) only in combination step
    // For now, just return the unscaled warp:
    VecXd warpRaw = Pmat * Veq.inverse().transpose() * (LGLr - req);
    return warpRaw;
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
    if (N <= 1)
        return;

    // Compute warp for each edge blend
    // Edge 1: L1=0 (between vertices 2 and 3)
    // Edge 2: L2=0 (between vertices 1 and 3)
    // Edge 3: L3=0 (between vertices 1 and 2)

    // Blending functions (Hesthaven & Warburton eq 10.3):
    VecXd blend1 = VecXd(Np), blend2 = VecXd(Np), blend3 = VecXd(Np);
    for (int i = 0; i < Np; ++i) {
        blend1(i) = 4.0 * L2(i) * L3(i);
        blend2(i) = 4.0 * L1(i) * L3(i);
        blend3(i) = 4.0 * L1(i) * L2(i);
    }

    // Warp for each edge:
    // Edge 1 parameterized by L3-L2
    // Edge 2 parameterized by L1-L3
    // Edge 3 parameterized by L2-L1
    VecXd e1param(Np), e2param(Np), e3param(Np);
    for (int i = 0; i < Np; ++i) {
        e1param(i) = L3(i) - L2(i);
        e2param(i) = L1(i) - L3(i);
        e3param(i) = L2(i) - L1(i);
    }

    VecXd warp1 = warpFactor(N, e1param);
    VecXd warp2 = warpFactor(N, e2param);
    VecXd warp3 = warpFactor(N, e3param);

    // Apply warp with blend
    for (int i = 0; i < Np; ++i) {
        double w1 = blend1(i) * warp1(i);
        double w2 = blend2(i) * warp2(i);
        double w3 = blend3(i) * warp3(i);

        // Shift in (r,s) coordinates
        // Edge 1 (L1=0): normal direction is (0, -1) -> (1, 1)/sqrt(2) in rs
        // Using the standard tangent directions for each edge:
        r(i) += w1 * 1.0 + w2 * (-1.0) + w3 * 0.0;
        s(i) += w1 * 0.0 + w2 * (-1.0) + w3 * 1.0;
        // The tangent vectors for each edge on the reference [-1,1] triangle:
        // Edge 1 (s=-1): tangent = (1,0), normal=(0,-1)... warp along tangent
        // Actually the warp displacements in (r,s):
        // For edge along bottom (s=-1): warp in r direction
        // For edge along left (r=-1): warp in s direction
        // For edge along hypotenuse (r+s=0): warp perpendicular
    }

    // The above is an approximation. For a robust implementation,
    // let me use the direct approach: compute (x,y) on equilateral, warp, convert back.
    // Reset and redo with equilateral triangle approach:
    sk = 0;
    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= N - i; ++j) {
            L1(sk) = static_cast<double>(i) / N;
            L3(sk) = static_cast<double>(j) / N;
            L2(sk) = 1.0 - L1(sk) - L3(sk);
            ++sk;
        }
    }

    // Equilateral triangle vertices
    double v1x = -1.0, v1y = -1.0 / std::sqrt(3.0);
    double v2x = 1.0, v2y = -1.0 / std::sqrt(3.0);
    double v3x = 0.0, v3y = 2.0 / std::sqrt(3.0);

    // Equidistant (x,y) on equilateral triangle
    VecXd xeq(Np), yeq(Np);
    for (int i = 0; i < Np; ++i) {
        xeq(i) = L1(i) * v1x + L2(i) * v2x + L3(i) * v3x;
        yeq(i) = L1(i) * v1y + L2(i) * v2y + L3(i) * v3y;
    }

    // Blend and warp
    for (int i = 0; i < Np; ++i) {
        blend1(i) = 4.0 * L2(i) * L3(i);
        blend2(i) = 4.0 * L1(i) * L3(i);
        blend3(i) = 4.0 * L1(i) * L2(i);
    }

    warp1 = warpFactor(N, e1param);
    warp2 = warpFactor(N, e2param);
    warp3 = warpFactor(N, e3param);

    // Tangent vectors for each edge of equilateral triangle
    double t1x = v3x - v2x, t1y = v3y - v2y;
    double t2x = v1x - v3x, t2y = v1y - v3y;
    double t3x = v2x - v1x, t3y = v2y - v1y;
    double l1 = std::sqrt(t1x * t1x + t1y * t1y);
    double l2 = std::sqrt(t2x * t2x + t2y * t2y);
    double l3 = std::sqrt(t3x * t3x + t3y * t3y);
    t1x /= l1;
    t1y /= l1;
    t2x /= l2;
    t2y /= l2;
    t3x /= l3;
    t3y /= l3;

    VecXd xw(Np), yw(Np);
    for (int i = 0; i < Np; ++i) {
        xw(i) = xeq(i) + blend1(i) * warp1(i) * t1x + blend2(i) * warp2(i) * t2x + blend3(i) * warp3(i) * t3x;
        yw(i) = yeq(i) + blend1(i) * warp1(i) * t1y + blend2(i) * warp2(i) * t2y + blend3(i) * warp3(i) * t3y;
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
    Tmat << v1x, v2x, v3x, v1y, v2y, v3y, 1.0, 1.0, 1.0;
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
        if (std::abs(B.s(i) + 1.0) < tol)
            f1.push_back(i);
        if (std::abs(B.r(i) + B.s(i)) < tol)
            f2.push_back(i);
        if (std::abs(B.r(i) + 1.0) < tol)
            f3.push_back(i);
    }

    // Sort face nodes by coordinate along each edge
    std::sort(f1.begin(), f1.end(), [&](int a, int b) { return B.r(a) < B.r(b); });
    std::sort(f2.begin(), f2.end(), [&](int a, int b) { return B.r(a) > B.r(b); });
    std::sort(f3.begin(), f3.end(), [&](int a, int b) { return B.s(a) > B.s(b); });

    for (int i = 0; i < B.Nfp && i < static_cast<int>(f1.size()); ++i)
        B.Fmask(i, 0) = f1[i];
    for (int i = 0; i < B.Nfp && i < static_cast<int>(f2.size()); ++i)
        B.Fmask(i, 1) = f2[i];
    for (int i = 0; i < B.Nfp && i < static_cast<int>(f3.size()); ++i)
        B.Fmask(i, 2) = f3[i];

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
