#include "DGGpuCompute.h"

namespace prs {
namespace dg {

DGGpuCompute::DGGpuCompute() = default;
DGGpuCompute::~DGGpuCompute() = default;

bool DGGpuCompute::initContext() {
    lastError_ = "GPU compute disabled (OpenGL 4.3 not available at build time)";
    available_ = false;
    return false;
}

bool DGGpuCompute::init2D(const Basis2D&, const Mesh2D&) { return false; }
void DGGpuCompute::resetFields2D() {}
void DGGpuCompute::computeRHS2D(float) {}
void DGGpuCompute::rkStageUpdate2D(float, float, float) {}
void DGGpuCompute::recordListenerPressure2D(int) {}
void DGGpuCompute::setSource2D(int, const VecXd&) {}
void DGGpuCompute::setListener2D(int, const VecXd&) {}

bool DGGpuCompute::init3D(const Basis3D&, const Mesh3D&) { return false; }
void DGGpuCompute::resetFields3D() {}
void DGGpuCompute::computeRHS3D(float) {}
void DGGpuCompute::rkStageUpdate3D(float, float, float) {}
void DGGpuCompute::recordListenerPressure3D(int) {}
void DGGpuCompute::setSource3D(int, const VecXd&) {}
void DGGpuCompute::setListener3D(int, const VecXd&) {}

void DGGpuCompute::allocImpulseResponse(int) {}
std::vector<float> DGGpuCompute::readImpulseResponse(int nSteps) { return std::vector<float>(nSteps, 0.0f); }
void DGGpuCompute::finish() {}

void DGGpuCompute::cleanup() {}

} // namespace dg
} // namespace prs
