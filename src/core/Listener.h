#pragma once

#include "Types.h"
#include <string>
#include <optional>

namespace prs {

struct Listener {
    Vec3f position = Vec3f::Zero();
    std::string name;
    std::optional<Vec3f> orientation;
    Color3i markerColor = {100, 100, 255};
};

} // namespace prs
