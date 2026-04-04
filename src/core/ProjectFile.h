#pragma once

#include "core/Types.h"
#include "core/PlacedPoint.h"
#include "core/Material.h"

#include <QString>
#include <QJsonObject>
#include <vector>
#include <optional>

namespace prs {

struct ProjectData {
    QString stlFilePath;
    float scaleFactor = 1.0f;
    int sampleRate = DEFAULT_SAMPLE_RATE;
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
