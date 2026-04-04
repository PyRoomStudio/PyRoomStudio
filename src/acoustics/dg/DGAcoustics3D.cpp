#include "DGAcoustics3D.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace prs {
namespace dg {

// LSRK4 coefficients (same as 2D solver)
static const double rk4a[5] = {0.0, -567301805773.0 / 1357537059087.0, -2404267990393.0 / 2016746695238.0,
                               -3550918686646.0 / 2091501179385.0, -1275806237668.0 / 842570457699.0};

static const double rk4b[5] = {1432997174477.0 / 9575080441755.0, 5161836677717.0 / 13612068292357.0,
                               1720146321549.0 / 2090206949498.0, 3134564353537.0 / 4481467310338.0,
                               2277821191437.0 / 14882151754819.0};

static const double rk4c[5] = {0.0, 1432997174477.0 / 9575080441755.0, 2526269341429.0 / 6820363962896.0,
                               2006345519317.0 / 3224571057899.0, 2802321613138.0 / 2924317926251.0};

DGAcoustics3D::DGAcoustics3D(const Basis3D& basis, const Mesh3D& mesh, const DGParams& params)
    : basis_(basis)
    , mesh_(mesh)
    , params_(params)
    , rho_(AIR_DENSITY)
    , c_(SOUND_SPEED) {
    sourceSigma_ = 1.0 / (M_PI * params_.maxFrequency);
    sourceT0_ = 4.0 * sourceSigma_;
}

void DGAcoustics3D::allocateScratch() {
    int Np = basis_.Np;
    int Nfp = basis_.Nfp;
    int Nfaces = basis_.Nfaces;
    int K = mesh_.K;
    int totalFaceNodes = Nfp * Nfaces;

    dpdr_.resize(Np, K);
    dpds_.resize(Np, K);
    dpdt_.resize(Np, K);
    duxdr_.resize(Np, K);
    duxds_.resize(Np, K);
    duxdt_.resize(Np, K);
    duydr_.resize(Np, K);
    duyds_.resize(Np, K);
    duydt_.resize(Np, K);
    duzdr_.resize(Np, K);
    duzds_.resize(Np, K);
    duzdt_.resize(Np, K);

    fluxP_.resize(totalFaceNodes, K);
    fluxUx_.resize(totalFaceNodes, K);
    fluxUy_.resize(totalFaceNodes, K);
    fluxUz_.resize(totalFaceNodes, K);

    scaledFluxP_.resize(totalFaceNodes, K);
    scaledFluxUx_.resize(totalFaceNodes, K);
    scaledFluxUy_.resize(totalFaceNodes, K);
    scaledFluxUz_.resize(totalFaceNodes, K);
}

int DGAcoustics3D::findElement(const Vec3d& pos) const {
    int bestElem = 0;
    double bestDist = 1e30;

    for (int k = 0; k < mesh_.K; ++k) {
        double cx = 0, cy = 0, cz = 0;
        for (int i = 0; i < basis_.Np; ++i) {
            cx += mesh_.x(i, k);
            cy += mesh_.y(i, k);
            cz += mesh_.z(i, k);
        }
        cx /= basis_.Np;
        cy /= basis_.Np;
        cz /= basis_.Np;
        double d = (pos.x() - cx) * (pos.x() - cx) + (pos.y() - cy) * (pos.y() - cy) + (pos.z() - cz) * (pos.z() - cz);
        if (d < bestDist) {
            bestDist = d;
            bestElem = k;
        }
    }
    return bestElem;
}

double DGAcoustics3D::interpolatePressure(const MatXd& p, int elem, const Vec3d& pos) const {
    double totalW = 0.0, val = 0.0;
    for (int i = 0; i < basis_.Np; ++i) {
        double dx = mesh_.x(i, elem) - pos.x();
        double dy = mesh_.y(i, elem) - pos.y();
        double dz = mesh_.z(i, elem) - pos.z();
        double w = 1.0 / (dx * dx + dy * dy + dz * dz + 1e-20);
        totalW += w;
        val += w * p(i, elem);
    }
    return val / totalW;
}

double DGAcoustics3D::computeTimeStep() const {
    double minH = 1e30;
    for (int k = 0; k < mesh_.K; ++k) {
        double vol = 0;
        for (int i = 0; i < basis_.Np; ++i)
            vol += std::abs(mesh_.J(i, k));
        vol /= basis_.Np;
        vol *= 4.0 / 3.0;

        double h = std::cbrt(6.0 * vol);
        minH = std::min(minH, h);
    }

    int N = basis_.N;
    return params_.cflFactor * minH / (c_ * (N + 1) * (N + 1));
}

void DGAcoustics3D::addSourceTerm(MatXd& rhsP, double time) {
    double arg = (time - sourceT0_) / sourceSigma_;
    double pulse = std::exp(-0.5 * arg * arg);

    for (int i = 0; i < basis_.Np; ++i)
        rhsP(i, sourceElem_) += pulse * sourceWeights_(i);
}

void DGAcoustics3D::computeRHS(const MatXd& p, const MatXd& ux, const MatXd& uy, const MatXd& uz, MatXd& rhsP,
                               MatXd& rhsUx, MatXd& rhsUy, MatXd& rhsUz, double time) {
    int Np = basis_.Np;
    int Nfp = basis_.Nfp;
    int Nfaces = basis_.Nfaces;
    int K = mesh_.K;
    int totalFaceNodes = Nfp * Nfaces;

    double c2 = c_ * c_;
    double invRho = 1.0 / rho_;

    // Reference-space derivatives (Eigen BLAS, outside OpenMP)
    dpdr_.noalias() = basis_.Dr * p;
    dpds_.noalias() = basis_.Ds * p;
    dpdt_.noalias() = basis_.Dt * p;
    duxdr_.noalias() = basis_.Dr * ux;
    duxds_.noalias() = basis_.Ds * ux;
    duxdt_.noalias() = basis_.Dt * ux;
    duydr_.noalias() = basis_.Dr * uy;
    duyds_.noalias() = basis_.Ds * uy;
    duydt_.noalias() = basis_.Dt * uy;
    duzdr_.noalias() = basis_.Dr * uz;
    duzds_.noalias() = basis_.Ds * uz;
    duzdt_.noalias() = basis_.Dt * uz;

// Volume terms: physical-space derivatives and RHS
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int k = 0; k < K; ++k) {
        for (int i = 0; i < Np; ++i) {
            double rxv = mesh_.rx(i, k), ryv = mesh_.ry(i, k), rzv = mesh_.rz(i, k);
            double sxv = mesh_.sx(i, k), syv = mesh_.sy(i, k), szv = mesh_.sz(i, k);
            double txv = mesh_.tx(i, k), tyv = mesh_.ty(i, k), tzv = mesh_.tz(i, k);

            double dpdx = rxv * dpdr_(i, k) + sxv * dpds_(i, k) + txv * dpdt_(i, k);
            double dpdy = ryv * dpdr_(i, k) + syv * dpds_(i, k) + tyv * dpdt_(i, k);
            double dpdz = rzv * dpdr_(i, k) + szv * dpds_(i, k) + tzv * dpdt_(i, k);

            double duxdx = rxv * duxdr_(i, k) + sxv * duxds_(i, k) + txv * duxdt_(i, k);
            double duydy = ryv * duydr_(i, k) + syv * duyds_(i, k) + tyv * duydt_(i, k);
            double duzdz = rzv * duzdr_(i, k) + szv * duzds_(i, k) + tzv * duzdt_(i, k);

            double divU = duxdx + duydy + duzdz;

            rhsP(i, k) = -rho_ * c2 * divU;
            rhsUx(i, k) = -invRho * dpdx;
            rhsUy(i, k) = -invRho * dpdy;
            rhsUz(i, k) = -invRho * dpdz;
        }
    }

    // Surface terms (parallelized over elements)
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
                double pM = p(nM, kM);
                double uxM = ux(nM, kM), uyM = uy(nM, kM), uzM = uz(nM, kM);

                double pP, uxP, uyP, uzP;
                double fnx = mesh_.nx(fid, k);
                double fny = mesh_.ny(fid, k);
                double fnz = mesh_.nz(fid, k);

                if (isBoundary) {
                    double Zwall = mesh_.boundaryImpedance(surfIdx);
                    if (Zwall < 1e-10)
                        Zwall = 1e10;
                    double R = (Zwall - Z) / (Zwall + Z);

                    double unM = uxM * fnx + uyM * fny + uzM * fnz;
                    double unP = -R * unM;
                    pP = R * pM;

                    double utxM = uxM - unM * fnx;
                    double utyM = uyM - unM * fny;
                    double utzM = uzM - unM * fnz;
                    uxP = unP * fnx + utxM;
                    uyP = unP * fny + utyM;
                    uzP = unP * fnz + utzM;
                } else {
                    int kP = idP / Np, nP = idP % Np;
                    pP = p(nP, kP);
                    uxP = ux(nP, kP);
                    uyP = uy(nP, kP);
                    uzP = uz(nP, kP);
                }

                double dp = pP - pM;
                double dunM = (uxP - uxM) * fnx + (uyP - uyM) * fny + (uzP - uzM) * fnz;

                fluxP_(fid, k) = 0.5 * (c_ * invZ * dp - c2 * dunM);
                double fluxUn = 0.5 * (-invRho * dp + c_ * Z * invRho * dunM);

                fluxUx_(fid, k) = fluxUn * fnx;
                fluxUy_(fid, k) = fluxUn * fny;
                fluxUz_(fid, k) = fluxUn * fnz;
            }
        }
    }

    // LIFT application (Eigen BLAS)
    scaledFluxP_.noalias() = fluxP_.cwiseProduct(mesh_.Fscale);
    scaledFluxUx_.noalias() = fluxUx_.cwiseProduct(mesh_.Fscale);
    scaledFluxUy_.noalias() = fluxUy_.cwiseProduct(mesh_.Fscale);
    scaledFluxUz_.noalias() = fluxUz_.cwiseProduct(mesh_.Fscale);

    rhsP.noalias() += basis_.LIFT * scaledFluxP_;
    rhsUx.noalias() += basis_.LIFT * scaledFluxUx_;
    rhsUy.noalias() += basis_.LIFT * scaledFluxUy_;
    rhsUz.noalias() += basis_.LIFT * scaledFluxUz_;

    addSourceTerm(rhsP, time);
}

AcousticsResult3D DGAcoustics3D::solve(const Vec3d& sourcePos, const Vec3d& listenerPos, ProgressCallback progress,
                                       CancelCheck cancel) {
    using Clock = std::chrono::steady_clock;
    AcousticsResult3D result;
    int Np = basis_.Np;
    int K = mesh_.K;

    allocateScratch();

    sourceElem_ = findElement(sourcePos);
    listenerElem_ = findElement(listenerPos);

    sourceWeights_.resize(Np);
    {
        double totalW = 0.0;
        for (int i = 0; i < Np; ++i) {
            double dx = mesh_.x(i, sourceElem_) - sourcePos.x();
            double dy = mesh_.y(i, sourceElem_) - sourcePos.y();
            double dz = mesh_.z(i, sourceElem_) - sourcePos.z();
            double h2 = std::abs(mesh_.J(0, sourceElem_));
            sourceWeights_(i) = std::exp(-(dx * dx + dy * dy + dz * dz) / (2.0 * h2 * 0.01));
            totalW += sourceWeights_(i);
        }
        if (totalW > 1e-30)
            sourceWeights_ /= totalW;
    }

    listenerWeights_.resize(Np);
    {
        double totalW = 0.0;
        for (int i = 0; i < Np; ++i) {
            double dx = mesh_.x(i, listenerElem_) - listenerPos.x();
            double dy = mesh_.y(i, listenerElem_) - listenerPos.y();
            double dz = mesh_.z(i, listenerElem_) - listenerPos.z();
            double w = 1.0 / (dx * dx + dy * dy + dz * dz + 1e-20);
            listenerWeights_(i) = w;
            totalW += w;
        }
        listenerWeights_ /= totalW;
    }

    double dt = computeTimeStep();
    double simDuration = params_.simulationDuration;
    if (simDuration <= 0.0) {
        double minX = mesh_.VX.minCoeff(), maxX = mesh_.VX.maxCoeff();
        double minY = mesh_.VY.minCoeff(), maxY = mesh_.VY.maxCoeff();
        double minZ = mesh_.VZ.minCoeff(), maxZ = mesh_.VZ.maxCoeff();
        double diag =
            std::sqrt((maxX - minX) * (maxX - minX) + (maxY - minY) * (maxY - minY) + (maxZ - minZ) * (maxZ - minZ));
        simDuration = std::clamp(3.0 * diag / c_, 0.1, 2.0);
    }

    int nSteps = static_cast<int>(std::ceil(simDuration / dt));
    result.sampleRate = 1.0 / dt;
    result.duration = nSteps * dt;
    result.numTimeSteps = nSteps;

    if (progress) {
        double memMB = static_cast<double>(Np) * K * sizeof(double) * 16 / (1024.0 * 1024.0);
        progress(0, "DG 3D: " + std::to_string(K) + " elems, " + std::to_string(nSteps) +
                        " steps, dt=" + std::to_string(dt * 1e6).substr(0, 5) + "us, " +
                        std::to_string(static_cast<int>(memMB)) + "MB field data");
    }

    MatXd p = MatXd::Zero(Np, K);
    MatXd ux = MatXd::Zero(Np, K);
    MatXd uy = MatXd::Zero(Np, K);
    MatXd uz = MatXd::Zero(Np, K);

    MatXd resP = MatXd::Zero(Np, K);
    MatXd resUx = MatXd::Zero(Np, K);
    MatXd resUy = MatXd::Zero(Np, K);
    MatXd resUz = MatXd::Zero(Np, K);

    MatXd rhsP(Np, K), rhsUx(Np, K), rhsUy(Np, K), rhsUz(Np, K);

    result.impulseResponse.resize(nSteps, 0.0f);

    auto wallStart = Clock::now();
    double stepTimeSec = 0.0;
    int warmupSteps = std::min(10, nSteps);

    for (int step = 0; step < nSteps; ++step) {
        if (cancel && cancel())
            break;
        double time = step * dt;

        for (int stage = 0; stage < 5; ++stage) {
            double stageTime = time + rk4c[stage] * dt;
            computeRHS(p, ux, uy, uz, rhsP, rhsUx, rhsUy, rhsUz, stageTime);

            resP = rk4a[stage] * resP + dt * rhsP;
            resUx = rk4a[stage] * resUx + dt * rhsUx;
            resUy = rk4a[stage] * resUy + dt * rhsUy;
            resUz = rk4a[stage] * resUz + dt * rhsUz;

            p += rk4b[stage] * resP;
            ux += rk4b[stage] * resUx;
            uy += rk4b[stage] * resUy;
            uz += rk4b[stage] * resUz;
        }

        double pListener = 0.0;
        for (int i = 0; i < Np; ++i)
            pListener += listenerWeights_(i) * p(i, listenerElem_);
        result.impulseResponse[step] = static_cast<float>(pListener);

        if (step == warmupSteps - 1) {
            auto now = Clock::now();
            double elapsed = std::chrono::duration<double>(now - wallStart).count();
            stepTimeSec = elapsed / warmupSteps;
        }

        if (progress && step >= warmupSteps) {
            int reportEvery = std::max(1, std::min(nSteps / 200, static_cast<int>(0.5 / std::max(stepTimeSec, 1e-9))));
            if (step % reportEvery == 0) {
                int pct = static_cast<int>(100.0 * step / nSteps);
                auto now = Clock::now();
                double elapsed = std::chrono::duration<double>(now - wallStart).count();
                double remaining = stepTimeSec * (nSteps - step);
                int etaMin = static_cast<int>(remaining / 60.0);
                int etaSec = static_cast<int>(remaining) % 60;

                std::string msg = "DG 3D: step " + std::to_string(step) + "/" + std::to_string(nSteps) + " [" +
                                  std::to_string(static_cast<int>(elapsed)) + "s elapsed" + ", ETA " +
                                  std::to_string(etaMin) + "m" + std::to_string(etaSec) + "s]";
                progress(pct, msg);
            }
        } else if (progress && step < warmupSteps && step % std::max(1, warmupSteps / 5) == 0) {
            progress(static_cast<int>(100.0 * step / nSteps), "DG 3D: warming up...");
        }
    }

    if (progress) {
        auto total = std::chrono::duration<double>(Clock::now() - wallStart).count();
        progress(100, "DG 3D: done in " + std::to_string(static_cast<int>(total)) + "s (" + std::to_string(nSteps) +
                          " steps)");
    }

    return result;
}

} // namespace dg
} // namespace prs
