#pragma once

#include "DGBasis2D.h"
#include "DGMesh2D.h"
#include "DGTypes.h"

#include <chrono>
#include <vector>

namespace prs {
namespace dg {

struct AcousticsResult2D {
    std::vector<float> impulseResponse;
    double sampleRate;
    double duration;
    int numTimeSteps;
};

class DGAcoustics2D {
  public:
    DGAcoustics2D(const Basis2D& basis, const Mesh2D& mesh, const DGParams& params);

    AcousticsResult2D solve(const Vec2d& sourcePos, const Vec2d& listenerPos, ProgressCallback progress = nullptr,
                            CancelCheck cancel = nullptr);

    int findElement(const Vec2d& pos) const;
    double computeTimeStep() const;

  private:
    void computeRHS(const MatXd& p, const MatXd& ux, const MatXd& uy, MatXd& rhsP, MatXd& rhsUx, MatXd& rhsUy,
                    double time);

    void addSourceTerm(MatXd& rhsP, double time);

    double interpolatePressure(const MatXd& p, int elem, const Vec2d& pos) const;

    const Basis2D& basis_;
    const Mesh2D& mesh_;
    DGParams params_;

    int sourceElem_;
    VecXd sourceWeights_;
    double sourceSigma_;
    double sourceT0_;

    int listenerElem_;
    VecXd listenerWeights_;

    double rho_;
    double c_;

    // Pre-allocated scratch matrices to avoid per-call heap allocation
    MatXd dpdr_, dpds_, duxdr_, duxds_, duydr_, duyds_;
    MatXd fluxP_, fluxUx_, fluxUy_;
    MatXd scaledFluxP_, scaledFluxUx_, scaledFluxUy_;

    void allocateScratch();
};

} // namespace dg
} // namespace prs
