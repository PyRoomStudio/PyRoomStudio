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
float DGGpuCompute::readListenerPressure2D() { return 0.0f; }
void DGGpuCompute::setSource2D(int, const VecXd&) {}
void DGGpuCompute::setListener2D(int, const VecXd&) {}

bool DGGpuCompute::init3D(const Basis3D&, const Mesh3D&) { return false; }
void DGGpuCompute::resetFields3D() {}
void DGGpuCompute::computeRHS3D(float) {}
void DGGpuCompute::rkStageUpdate3D(float, float, float) {}
float DGGpuCompute::readListenerPressure3D() { return 0.0f; }
void DGGpuCompute::setSource3D(int, const VecXd&) {}
void DGGpuCompute::setListener3D(int, const VecXd&) {}

void DGGpuCompute::cleanup() {}

} // namespace dg
} // namespace prs
