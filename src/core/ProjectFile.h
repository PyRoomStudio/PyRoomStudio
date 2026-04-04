#pragma once

#include "core/Material.h"
#include "core/PlacedPoint.h"
#include "core/Types.h"

#include <QJsonObject>
#include <QString>

#include <optional>
#include <vector>

namespace prs {

struct ProjectData {
    QString stlFilePath;
    float scaleFactor = 1.0f;
    std::vector<Color3f> surfaceColors;
    std::vector<std::optional<Material>> surfaceMaterials;
    std::vector<PlacedPoint> placedPoints;
    QString soundSourceFile;
};

class ProjectFile {
  public:
    static bool save(const QString& filepath, const ProjectData& data);
    static std::optional<ProjectData> load(const QString& filepath);
};

} // namespace prs
