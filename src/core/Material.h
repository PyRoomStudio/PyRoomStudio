#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <map>

namespace prs {

struct Material {
    std::string name;
    float energyAbsorption = 0.2f;
    float scattering = 0.1f;
    Color3i color = {160, 160, 160};
    std::string category;
};

struct MaterialCategory {
    std::string name;
    std::vector<Material> materials;
};

const std::vector<MaterialCategory>& getAcousticMaterials();

} // namespace prs
