#pragma once

#include "DGBasis3D.h"
#include "DGMesh3D.h"
#include "DGTypes.h"

#include <vector>

namespace prs {
namespace dg {

struct AcousticsResult3D {
    std::vector<float> impulseResponse;
    double sampleRate;
    double duration;
    int numTimeSteps;
};

class DGAcoustics3D {
  public:
    DGAcoustics3D(const Basis3D& basis, const Mesh3D& mesh, const DGParams& params);

    AcousticsResult3D solve(const Vec3d& sourcePos, const Vec3d& listenerPos, ProgressCallback progress = nullptr,
                            CancelCheck cancel = nullptr);

    int findElement(const Vec3d& pos) const;
    double computeTimeStep() const;

  private:
    void computeRHS(const MatXd& p, const MatXd& ux, const MatXd& uy, const MatXd& uz, MatXd& rhsP, MatXd& rhsUx,
                    MatXd& rhsUy, MatXd& rhsUz, double time);

    void addSourceTerm(MatXd& rhsP, double time);

    double interpolatePressure(const MatXd& p, int elem, const Vec3d& pos) const;

    const Basis3D& basis_;
    const Mesh3D& mesh_;
    DGParams params_;

    int sourceElem_;
    VecXd sourceWeights_;
    double sourceSigma_;
    double sourceT0_;

    int listenerElem_;
    VecXd listenerWeights_;

    double rho_;
    double c_;

    // Pre-allocated scratch matrices
    MatXd dpdr_, dpds_, dpdt_;
    MatXd duxdr_, duxds_, duxdt_;
    MatXd duydr_, duyds_, duydt_;
    MatXd duzdr_, duzds_, duzdt_;
    MatXd fluxP_, fluxUx_, fluxUy_, fluxUz_;
    MatXd scaledFluxP_, scaledFluxUx_, scaledFluxUy_, scaledFluxUz_;

    void allocateScratch();
};

} // namespace dg
} // namespace prs
