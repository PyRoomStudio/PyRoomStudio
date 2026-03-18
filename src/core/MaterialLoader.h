#pragma once

#include "Material.h"
#include <QString>
#include <optional>

namespace prs {

class MaterialLoader {
public:
    static std::vector<MaterialCategory> loadFromDirectory(const QString& dirPath);
    static std::optional<Material> loadFromFile(const QString& filePath);
    static bool saveToFile(const QString& filePath, const Material& material);
};

} // namespace prs
