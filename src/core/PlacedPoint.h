#pragma once

#include "Types.h"
#include <string>

namespace prs {

struct PlacedPoint {
    Vec3f surfacePoint = Vec3f::Zero();
    Vec3f normal       = Vec3f::UnitZ();
    float distance     = 0.0f;
    Color3f color      = {0.2f, 0.8f, 0.2f};
    std::string pointType = POINT_TYPE_NONE;

    Vec3f getPosition() const {
        return surfacePoint + normal * distance;
    }
};

} // namespace prs
