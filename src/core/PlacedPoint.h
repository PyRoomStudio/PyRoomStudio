#pragma once

#include "Types.h"

#include <cmath>
#include <string>

namespace prs {

struct PlacedPoint {
    Vec3f surfacePoint = Vec3f::Zero();
    Vec3f normal = Vec3f::UnitZ();
    float distance = 0.0f;
    Color3f color = {0.2f, 0.8f, 0.2f};
    std::string pointType = POINT_TYPE_NONE;
    std::string name;
    float volume = 1.0f;
    std::string audioFile;
    float orientationYaw = 0.0f; // degrees, listener facing direction (rotation around Y axis)

    Vec3f getPosition() const { return surfacePoint + normal * distance; }

    Vec3f getForwardDirection() const {
        float rad = orientationYaw * static_cast<float>(M_PI) / 180.0f;
        return Vec3f(std::cos(rad), std::sin(rad), 0.0f);
    }
};

} // namespace prs
