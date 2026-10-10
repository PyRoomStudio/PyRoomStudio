#pragma once

#include <Eigen/Core>
#include <Eigen/Dense>
#include <vector>
#include <string>
#include <functional>

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

// LSRK4 (Low-Storage Runge-Kutta, 5 stages) coefficients
constexpr double LSRK4_A[5] = {
    0.0,
    -567301805773.0  / 1357537059087.0,
    -2404267990393.0 / 2016746695238.0,
    -3550918686646.0 / 2091501179385.0,
    -1275806237668.0 / 842570457699.0
};

constexpr double LSRK4_B[5] = {
    1432997174477.0 / 9575080441755.0,
    5161836677717.0 / 13612068292357.0,
    1720146321549.0 / 2090206949498.0,
    3134564353537.0 / 4481467310338.0,
    2277821191437.0 / 14882151754819.0
};

constexpr double LSRK4_C[5] = {
    0.0,
    1432997174477.0 / 9575080441755.0,
    2526269341429.0 / 6820363962896.0,
    2006345519317.0 / 3224571057899.0,
    2802321613138.0 / 2924317926251.0
};

enum class DGMethod { DG_2D, DG_3D };

struct DGParams {
    DGMethod method = DGMethod::DG_2D;
    int polynomialOrder = 3;
    double maxFrequency = 1000.0;
    double cflFactor = 0.5;
    double simulationDuration = 0.0; // 0 = auto from room size
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
