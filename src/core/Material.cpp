#include "Material.h"

namespace prs {

const std::vector<MaterialCategory>& getAcousticMaterials() {
    static const std::vector<MaterialCategory> materials = {
        {"Wall Materials", {
            {"Brick",      0.03f, 0.1f, {178, 102, 85},  "Wall Materials"},
            {"Concrete",   0.02f, 0.1f, {160, 160, 160}, "Wall Materials"},
            {"Drywall",    0.10f, 0.1f, {240, 235, 225}, "Wall Materials"},
            {"Plaster",    0.04f, 0.1f, {245, 240, 230}, "Wall Materials"},
            {"Glass",      0.03f, 0.1f, {200, 230, 245}, "Wall Materials"},
            {"Wood Panel", 0.10f, 0.1f, {160, 120, 80},  "Wall Materials"},
        }},
        {"Floor Materials", {
            {"Hardwood",     0.10f, 0.1f, {140, 90, 55},   "Floor Materials"},
            {"Carpet Thick", 0.65f, 0.1f, {100, 80, 120},  "Floor Materials"},
            {"Carpet Thin",  0.35f, 0.1f, {150, 130, 160}, "Floor Materials"},
            {"Tile",         0.02f, 0.1f, {210, 200, 180}, "Floor Materials"},
            {"Linoleum",     0.03f, 0.1f, {180, 200, 170}, "Floor Materials"},
            {"Concrete",     0.02f, 0.1f, {145, 145, 145}, "Floor Materials"},
        }},
        {"Ceiling Materials", {
            {"Acoustic",  0.70f, 0.1f, {90, 90, 110},   "Ceiling Materials"},
            {"Plaster",   0.04f, 0.1f, {250, 250, 245}, "Ceiling Materials"},
            {"Wood",      0.10f, 0.1f, {180, 140, 100}, "Ceiling Materials"},
            {"Metal",     0.05f, 0.1f, {190, 195, 200}, "Ceiling Materials"},
            {"Suspended", 0.50f, 0.1f, {220, 220, 230}, "Ceiling Materials"},
        }},
        {"Soft Furnish", {
            {"Heavy Drape", 0.55f, 0.1f, {130, 50, 70},   "Soft Furnish"},
            {"Light Drape", 0.15f, 0.1f, {230, 200, 180}, "Soft Furnish"},
            {"Upholstery",  0.45f, 0.1f, {80, 100, 130},  "Soft Furnish"},
            {"Fabric",      0.60f, 0.1f, {140, 160, 80},  "Soft Furnish"},
        }},
    };
    return materials;
}

} // namespace prs
