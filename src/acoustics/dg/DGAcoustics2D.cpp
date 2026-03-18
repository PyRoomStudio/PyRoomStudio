#include "DGAcoustics2D.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <chrono>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace prs {
namespace dg {

// LSRK4 (Low-Storage Runge-Kutta, 5 stages) coefficients
static const double rk4a[5] = {
    0.0,
    -567301805773.0  / 1357537059087.0,
    -2404267990393.0 / 2016746695238.0,
    -3550918686646.0 / 2091501179385.0,
    -1275806237668.0 / 842570457699.0
};

static const double rk4b[5] = {
    1432997174477.0 / 9575080441755.0,
    5161836677717.0 / 13612068292357.0,
    1720146321549.0 / 2090206949498.0,
    3134564353537.0 / 4481467310338.0,
    2277821191437.0 / 14882151754819.0
};

static const double rk4c[5] = {
    0.0,
    1432997174477.0 / 9575080441755.0,
    2526269341429.0 / 6820363962896.0,
    2006345519317.0 / 3224571057899.0,
    2802321613138.0 / 2924317926251.0
};

DGAcoustics2D::DGAcoustics2D(const Basis2D& basis, const Mesh2D& mesh, const DGParams& params)
    : basis_(basis), mesh_(mesh), params_(params),
      rho_(AIR_DENSITY), c_(SOUND_SPEED)
{
    sourceSigma_ = 1.0 / (M_PI * params_.maxFrequency);
    sourceT0_ = 4.0 * sourceSigma_;
}

void DGAcoustics2D::allocateScratch() {
    int Np = basis_.Np;
    int Nfp = basis_.Nfp;
    int Nfaces = basis_.Nfaces;
    int K = mesh_.K;
    int totalFaceNodes = Nfp * Nfaces;

    dpdr_.resize(Np, K);
    dpds_.resize(Np, K);
    duxdr_.resize(Np, K);
    duxds_.resize(Np, K);
    duydr_.resize(Np, K);
    duyds_.resize(Np, K);

    fluxP_.resize(totalFaceNodes, K);
    fluxUx_.resize(totalFaceNodes, K);
    fluxUy_.resize(totalFaceNodes, K);

    scaledFluxP_.resize(totalFaceNodes, K);
    scaledFluxUx_.resize(totalFaceNodes, K);
    scaledFluxUy_.resize(totalFaceNodes, K);
}

int DGAcoustics2D::findElement(const Vec2d& pos) const {
    int bestElem = 0;
    double bestDist = 1e30;

    for (int k = 0; k < mesh_.K; ++k) {
        double cx = 0, cy = 0;
        for (int i = 0; i < basis_.Np; ++i) {
            cx += mesh_.x(i, k);
            cy += mesh_.y(i, k);
        }
        cx /= basis_.Np;
        cy /= basis_.Np;
        double d = (pos.x() - cx) * (pos.x() - cx) + (pos.y() - cy) * (pos.y() - cy);
        if (d < bestDist) {
            bestDist = d;
            bestElem = k;
        }
    }
    return bestElem;
}

double DGAcoustics2D::interpolatePressure(const MatXd& p, int elem, const Vec2d& pos) const {
    double totalW = 0.0, val = 0.0;
    for (int i = 0; i < basis_.Np; ++i) {
        double dx = mesh_.x(i, elem) - pos.x();
        double dy = mesh_.y(i, elem) - pos.y();
        double d2 = dx * dx + dy * dy + 1e-20;
        double w = 1.0 / d2;
        totalW += w;
        val += w * p(i, elem);
    }
    return val / totalW;
}

double DGAcoustics2D::computeTimeStep() const {
    double minH = 1e30;
    for (int k = 0; k < mesh_.K; ++k) {
        double area = 0;
        for (int i = 0; i < basis_.Np; ++i)
            area += std::abs(mesh_.J(i, k));
        area /= basis_.Np;
        area *= 2.0;

        double x0 = mesh_.VX(mesh_.EToV(k, 0)), y0 = mesh_.VY(mesh_.EToV(k, 0));
        double x1 = mesh_.VX(mesh_.EToV(k, 1)), y1 = mesh_.VY(mesh_.EToV(k, 1));
        double x2 = mesh_.VX(mesh_.EToV(k, 2)), y2 = mesh_.VY(mesh_.EToV(k, 2));

        double e0 = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
        double e1 = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
        double e2 = std::sqrt((x0 - x2) * (x0 - x2) + (y0 - y2) * (y0 - y2));
        double perimeter = e0 + e1 + e2;

        double h = 2.0 * area / perimeter;
        minH = std::min(minH, h);
    }

    int N = basis_.N;
    double dt = params_.cflFactor * minH / (c_ * (N + 1) * (N + 1));
    return dt;
}

void DGAcoustics2D::addSourceTerm(MatXd& rhsP, double time) {
    double arg = (time - sourceT0_) / sourceSigma_;
    double pulse = std::exp(-0.5 * arg * arg);

    for (int i = 0; i < basis_.Np; ++i) {
        rhsP(i, sourceElem_) += pulse * sourceWeights_(i);
    }
}

void DGAcoustics2D::computeRHS(
    const MatXd& p, const MatXd& ux, const MatXd& uy,
    MatXd& rhsP, MatXd& rhsUx, MatXd& rhsUy,
    double time)
{
    int Np = basis_.Np;
    int Nfp = basis_.Nfp;
    int Nfaces = basis_.Nfaces;
    int K = mesh_.K;
    int totalFaceNodes = Nfp * Nfaces;

    double c2 = c_ * c_;
    double invRho = 1.0 / rho_;

    // Compute reference-space derivatives via matrix multiply (OUTSIDE OpenMP)
    // These are (Np x Np) * (Np x K) -> (Np x K), handled by Eigen BLAS
    dpdr_.noalias()  = basis_.Dr * p;
    dpds_.noalias()  = basis_.Ds * p;
    duxdr_.noalias() = basis_.Dr * ux;
    duxds_.noalias() = basis_.Ds * ux;
    duydr_.noalias() = basis_.Dr * uy;
    duyds_.noalias() = basis_.Ds * uy;

    // Volume terms: transform to physical space and compute RHS
    #ifdef _OPENMP
    #pragma omp parallel for schedule(static)
    #endif
    for (int k = 0; k < K; ++k) {
        for (int i = 0; i < Np; ++i) {
            double rxv = mesh_.rx(i, k), ryv = mesh_.ry(i, k);
            double sxv = mesh_.sx(i, k), syv = mesh_.sy(i, k);

            double dpdx = rxv * dpdr_(i, k) + sxv * dpds_(i, k);
            double dpdy = ryv * dpdr_(i, k) + syv * dpds_(i, k);

            double duxdx = rxv * duxdr_(i, k) + sxv * duxds_(i, k);
            double duydy = ryv * duydr_(i, k) + syv * duyds_(i, k);

            rhsP(i, k)  = -rho_ * c2 * (duxdx + duydy);
            rhsUx(i, k) = -invRho * dpdx;
            rhsUy(i, k) = -invRho * dpdy;
        }
    }

    // Surface flux computation (parallelized over elements)
    double Z = rho_ * c_;
    double invZ = 1.0 / Z;

    #ifdef _OPENMP
    #pragma omp parallel for schedule(static)
    #endif
    for (int k = 0; k < K; ++k) {
        for (int f = 0; f < Nfaces; ++f) {
            bool isBoundary = (mesh_.EToE(k, f) == k && mesh_.EToF(k, f) == f);

            for (int i = 0; i < Nfp; ++i) {
                int fid = f * Nfp + i;
                int surfIdx = k * totalFaceNodes + fid;
                int idM = mesh_.vmapM(surfIdx);
                int idP = mesh_.vmapP(surfIdx);

                int kM = idM / Np, nM = idM % Np;
                double pM  = p(nM, kM);
                double uxM = ux(nM, kM);
                double uyM = uy(nM, kM);

                double pP, uxP, uyP;

                double fnx = mesh_.nx(fid, k);
                double fny = mesh_.ny(fid, k);

                if (isBoundary) {
                    double Zwall = mesh_.boundaryImpedance(surfIdx);
                    if (Zwall < 1e-10) Zwall = 1e10;

                    double R = (Zwall - Z) / (Zwall + Z);

                    double unM = uxM * fnx + uyM * fny;
                    double unP = -R * unM;
                    pP = R * pM;

                    double utxM = uxM - unM * fnx;
                    double utyM = uyM - unM * fny;
                    uxP = unP * fnx + utxM;
                    uyP = unP * fny + utyM;
                } else {
                    int kP = idP / Np, nP = idP % Np;
                    pP  = p(nP, kP);
                    uxP = ux(nP, kP);
                    uyP = uy(nP, kP);
                }

                double dp = pP - pM;
                double dunM = (uxP - uxM) * fnx + (uyP - uyM) * fny;

                fluxP_(fid, k) = 0.5 * (c_ * invZ * dp - c2 * dunM);
                double fluxUn  = 0.5 * (-invRho * dp + c_ * Z * invRho * dunM);

                fluxUx_(fid, k) = fluxUn * fnx;
                fluxUy_(fid, k) = fluxUn * fny;
            }
        }
    }

    // Scale flux by Fscale and apply LIFT (outside OpenMP — Eigen BLAS)
    scaledFluxP_.noalias()  = fluxP_.cwiseProduct(mesh_.Fscale);
    scaledFluxUx_.noalias() = fluxUx_.cwiseProduct(mesh_.Fscale);
    scaledFluxUy_.noalias() = fluxUy_.cwiseProduct(mesh_.Fscale);

    rhsP.noalias()  += basis_.LIFT * scaledFluxP_;
    rhsUx.noalias() += basis_.LIFT * scaledFluxUx_;
    rhsUy.noalias() += basis_.LIFT * scaledFluxUy_;

    addSourceTerm(rhsP, time);
}

AcousticsResult2D DGAcoustics2D::solve(
    const Vec2d& sourcePos,
    const Vec2d& listenerPos,
    ProgressCallback progress,
    CancelCheck cancel)
{
    using Clock = std::chrono::steady_clock;
    AcousticsResult2D result;
    int Np = basis_.Np;
    int K = mesh_.K;

    // Pre-allocate all scratch matrices once
    allocateScratch();

    sourceElem_ = findElement(sourcePos);
    listenerElem_ = findElement(listenerPos);

    // Source injection weights
    sourceWeights_.resize(Np);
    {
        double totalW = 0.0;
        for (int i = 0; i < Np; ++i) {
            double dx = mesh_.x(i, sourceElem_) - sourcePos.x();
            double dy = mesh_.y(i, sourceElem_) - sourcePos.y();
            double d2 = dx * dx + dy * dy;
            double h2 = std::abs(mesh_.J(0, sourceElem_));
            sourceWeights_(i) = std::exp(-d2 / (2.0 * h2 * 0.01));
            totalW += sourceWeights_(i);
        }
        if (totalW > 1e-30)
            sourceWeights_ /= totalW;
    }

    // Listener interpolation weights
    listenerWeights_.resize(Np);
    {
        double totalW = 0.0;
        for (int i = 0; i < Np; ++i) {
            double dx = mesh_.x(i, listenerElem_) - listenerPos.x();
            double dy = mesh_.y(i, listenerElem_) - listenerPos.y();
            double w = 1.0 / (dx * dx + dy * dy + 1e-20);
            listenerWeights_(i) = w;
            totalW += w;
        }
        listenerWeights_ /= totalW;
    }

    // Time stepping
    double dt = computeTimeStep();
    double simDuration = params_.simulationDuration;
    if (simDuration <= 0.0) {
        double minX = mesh_.VX.minCoeff(), maxX = mesh_.VX.maxCoeff();
        double minY = mesh_.VY.minCoeff(), maxY = mesh_.VY.maxCoeff();
        double roomDiag = std::sqrt((maxX - minX) * (maxX - minX)
                                   + (maxY - minY) * (maxY - minY));
        simDuration = 3.0 * roomDiag / c_;
        simDuration = std::max(simDuration, 0.1);
        simDuration = std::min(simDuration, 2.0);
    }

    int nSteps = static_cast<int>(std::ceil(simDuration / dt));
    result.sampleRate = 1.0 / dt;
    result.duration = nSteps * dt;
    result.numTimeSteps = nSteps;

    if (progress) {
        double memMB = static_cast<double>(Np) * K * sizeof(double) * 9 / (1024.0 * 1024.0);
        progress(0, "DG 2D: " + std::to_string(K) + " elems, "
                   + std::to_string(nSteps) + " steps, dt="
                   + std::to_string(dt * 1e6).substr(0, 5) + "us, "
                   + std::to_string(static_cast<int>(memMB)) + "MB field data");
    }

    // Initialize fields
    MatXd p  = MatXd::Zero(Np, K);
    MatXd ux = MatXd::Zero(Np, K);
    MatXd uy = MatXd::Zero(Np, K);

    // RK residual storage
    MatXd resP  = MatXd::Zero(Np, K);
    MatXd resUx = MatXd::Zero(Np, K);
    MatXd resUy = MatXd::Zero(Np, K);

    // RHS storage (pre-allocated, reused every stage)
    MatXd rhsP(Np, K), rhsUx(Np, K), rhsUy(Np, K);

    result.impulseResponse.resize(nSteps, 0.0f);

    // Timing: measure first few steps to estimate total time
    auto wallStart = Clock::now();
    double stepTimeSec = 0.0;
    int warmupSteps = std::min(10, nSteps);

    for (int step = 0; step < nSteps; ++step) {
        if (cancel && cancel()) break;

        double time = step * dt;

        // LSRK4 stages
        for (int stage = 0; stage < 5; ++stage) {
            double stageTime = time + rk4c[stage] * dt;

            computeRHS(p, ux, uy, rhsP, rhsUx, rhsUy, stageTime);

            resP  = rk4a[stage] * resP  + dt * rhsP;
            resUx = rk4a[stage] * resUx + dt * rhsUx;
            resUy = rk4a[stage] * resUy + dt * rhsUy;

            p  += rk4b[stage] * resP;
            ux += rk4b[stage] * resUx;
            uy += rk4b[stage] * resUy;
        }

        // Record pressure at listener
        double pListener = 0.0;
        for (int i = 0; i < Np; ++i)
            pListener += listenerWeights_(i) * p(i, listenerElem_);
        result.impulseResponse[step] = static_cast<float>(pListener);

        // After warmup, compute ETA; then report every ~2 seconds of wall time
        if (step == warmupSteps - 1) {
            auto now = Clock::now();
            double elapsed = std::chrono::duration<double>(now - wallStart).count();
            stepTimeSec = elapsed / warmupSteps;
        }

        if (progress && step >= warmupSteps) {
            // Report every ~500ms worth of steps (but at least every 1% progress)
            int reportEvery = std::max(1, std::min(nSteps / 200,
                                static_cast<int>(0.5 / std::max(stepTimeSec, 1e-9))));
            if (step % reportEvery == 0) {
                int pct = static_cast<int>(100.0 * step / nSteps);
                auto now = Clock::now();
                double elapsed = std::chrono::duration<double>(now - wallStart).count();
                double remaining = stepTimeSec * (nSteps - step);
                int etaMin = static_cast<int>(remaining / 60.0);
                int etaSec = static_cast<int>(remaining) % 60;

                std::string msg = "DG 2D: step " + std::to_string(step)
                    + "/" + std::to_string(nSteps)
                    + " [" + std::to_string(static_cast<int>(elapsed)) + "s elapsed"
                    + ", ETA " + std::to_string(etaMin) + "m" + std::to_string(etaSec) + "s]";
                progress(pct, msg);
            }
        } else if (progress && step < warmupSteps && step % std::max(1, warmupSteps / 5) == 0) {
            progress(static_cast<int>(100.0 * step / nSteps), "DG 2D: warming up...");
        }
    }

    if (progress) {
        auto total = std::chrono::duration<double>(Clock::now() - wallStart).count();
        progress(100, "DG 2D: done in " + std::to_string(static_cast<int>(total)) + "s ("
                      + std::to_string(nSteps) + " steps)");
    }

    return result;
}

} // namespace dg
} // namespace prs
