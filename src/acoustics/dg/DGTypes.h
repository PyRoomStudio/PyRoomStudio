#pragma once

#include <Eigen/Core>
#include <Eigen/Dense>
#include <functional>
#include <string>
#include <vector>

namespace prs {
namespace dg {

using MatXd = Eigen::MatrixXd;
using VecXd = Eigen::VectorXd;
using MatXi = Eigen::MatrixXi;
using VecXi = Eigen::VectorXi;
using Vec2d = Eigen::Vector2d;
using Vec3d = Eigen::Vector3d;

constexpr double AIR_DENSITY = 1.225;
constexpr double SOUND_SPEED = 343.0;

enum class DGMethod { DG_2D, DG_3D };

struct DGParams {
    DGMethod method = DGMethod::DG_2D;
    int polynomialOrder = 3;
    double maxFrequency = 1000.0;
    double cflFactor = 0.5;
    double simulationDuration = 0.0; // 0 = auto from room size
};

struct BoundaryEdge2D {
    int element;
    int localFace;
    double impedance;
};

struct BoundaryFace3D {
    int element;
    int localFace;
    double impedance;
};

using ProgressCallback = std::function<void(int, const std::string&)>;
using CancelCheck = std::function<bool()>;

inline double absorptionToImpedance(double alpha) {
    alpha = std::clamp(alpha, 0.001, 0.999);
    double sqrtTerm = std::sqrt(1.0 - alpha);
    return AIR_DENSITY * SOUND_SPEED * (1.0 + sqrtTerm) / (1.0 - sqrtTerm);
}

} // namespace dg
} // namespace prs
