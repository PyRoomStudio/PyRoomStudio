#pragma once

#include <array>
#include <cstdint>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <tuple>
#include <vector>

namespace prs {

using Vec2f = Eigen::Vector2f;
using Vec3f = Eigen::Vector3f;
using Vec4f = Eigen::Vector4f;
using Mat3f = Eigen::Matrix3f;
using Mat4f = Eigen::Matrix4f;

using Color3i = std::array<int, 3>;
using Color3f = std::array<float, 3>;
using Color4f = std::array<float, 4>;

struct Triangle {
    Vec3f v0, v1, v2;
    Vec3f normal;
};

using Edge = std::pair<std::tuple<float, float, float>, std::tuple<float, float, float>>;

inline std::tuple<float, float, float> toKey(const Vec3f& v) {
    return {v.x(), v.y(), v.z()};
}

inline Edge makeEdge(const Vec3f& a, const Vec3f& b) {
    auto ka = toKey(a);
    auto kb = toKey(b);
    return (ka < kb) ? Edge{ka, kb} : Edge{kb, ka};
}

constexpr const char* POINT_TYPE_NONE = "none";
constexpr const char* POINT_TYPE_SOURCE = "source";
constexpr const char* POINT_TYPE_LISTENER = "listener";

constexpr int DEFAULT_SAMPLE_RATE = 44100;
constexpr float SPEED_OF_SOUND = 343.0f;
constexpr int DEFAULT_MAX_ORDER = 3;
constexpr int DEFAULT_N_RAYS = 10000;
constexpr float DEFAULT_ENERGY_ABSORPTION = 0.2f;
constexpr float DEFAULT_SCATTERING = 0.1f;

} // namespace prs
