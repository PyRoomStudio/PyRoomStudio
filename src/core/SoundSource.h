#pragma once

#include "Types.h"
#include <string>

namespace prs {

struct SoundSource {
    Vec3f position = Vec3f::Zero();
    std::string audioFile;
    float volume = 1.0f;
    std::string name;
    Color3i markerColor = {255, 100, 100};
};

} // namespace prs
