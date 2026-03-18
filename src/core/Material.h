#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <array>
#include <map>

namespace prs {

constexpr int NUM_FREQ_BANDS = 6;
constexpr std::array<int, NUM_FREQ_BANDS> FREQ_BANDS = {125, 250, 500, 1000, 2000, 4000};

struct Material {
    std::string name;
    std::string category;
    std::array<float, NUM_FREQ_BANDS> absorption = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f};
    float scattering = 0.1f;
    Color3i color = {160, 160, 160};
    std::string texturePath;
    std::string thickness;

    float averageAbsorption() const;
    float absorptionAt(int freqHz) const;
};

struct MaterialCategory {
    std::string name;
    std::vector<Material> materials;
};

} // namespace prs
