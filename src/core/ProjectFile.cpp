#include "ProjectFile.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>

namespace prs {

static QJsonObject vec3ToJson(const Vec3f& v) {
    QJsonObject o;
    o["x"] = static_cast<double>(v.x());
    o["y"] = static_cast<double>(v.y());
    o["z"] = static_cast<double>(v.z());
    return o;
}

static Vec3f jsonToVec3(const QJsonObject& o) {
    return Vec3f(o["x"].toDouble(), o["y"].toDouble(), o["z"].toDouble());
}

static QJsonArray color3fToJson(const Color3f& c) {
    QJsonArray a;
    a.append(static_cast<double>(c[0]));
    a.append(static_cast<double>(c[1]));
    a.append(static_cast<double>(c[2]));
    return a;
}

static Color3f jsonToColor3f(const QJsonArray& a) {
    return {static_cast<float>(a[0].toDouble()),
            static_cast<float>(a[1].toDouble()),
            static_cast<float>(a[2].toDouble())};
}

bool ProjectFile::save(const QString& filepath, const ProjectData& data) {
    QJsonObject root;
    root["version"] = 1;
    root["stlFilePath"] = data.stlFilePath;
    root["scaleFactor"] = static_cast<double>(data.scaleFactor);
    root["soundSourceFile"] = data.soundSourceFile;

    QJsonArray colorsArr;
    for (auto& c : data.surfaceColors)
        colorsArr.append(color3fToJson(c));
    root["surfaceColors"] = colorsArr;

    QJsonArray matsArr;
    for (auto& m : data.surfaceMaterials) {
        if (m.has_value()) {
            QJsonObject mo;
            mo["name"] = QString::fromStdString(m->name);
            mo["energyAbsorption"] = static_cast<double>(m->energyAbsorption);
            mo["scattering"] = static_cast<double>(m->scattering);
            matsArr.append(mo);
        } else {
            matsArr.append(QJsonValue());
        }
    }
    root["surfaceMaterials"] = matsArr;

    QJsonArray pointsArr;
    for (auto& pt : data.placedPoints) {
        QJsonObject po;
        po["surfacePoint"] = vec3ToJson(pt.surfacePoint);
        po["normal"] = vec3ToJson(pt.normal);
        po["distance"] = static_cast<double>(pt.distance);
        po["color"] = color3fToJson(pt.color);
        po["pointType"] = QString::fromStdString(pt.pointType);
        po["name"] = QString::fromStdString(pt.name);
        po["volume"] = static_cast<double>(pt.volume);
        po["audioFile"] = QString::fromStdString(pt.audioFile);
        po["orientationYaw"] = static_cast<double>(pt.orientationYaw);
        pointsArr.append(po);
    }
    root["placedPoints"] = pointsArr;

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(QJsonDocument(root).toJson());
    return true;
}

std::optional<ProjectData> ProjectFile::load(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError)
        return std::nullopt;

    QJsonObject root = doc.object();
    ProjectData data;

    data.stlFilePath     = root["stlFilePath"].toString();
    data.scaleFactor     = static_cast<float>(root["scaleFactor"].toDouble(1.0));
    data.soundSourceFile = root["soundSourceFile"].toString();

    for (auto val : root["surfaceColors"].toArray())
        data.surfaceColors.push_back(jsonToColor3f(val.toArray()));

    for (auto val : root["surfaceMaterials"].toArray()) {
        if (val.isNull() || val.isUndefined()) {
            data.surfaceMaterials.push_back(std::nullopt);
        } else {
            QJsonObject mo = val.toObject();
            Material m;
            m.name = mo["name"].toString().toStdString();
            m.energyAbsorption = static_cast<float>(mo["energyAbsorption"].toDouble());
            m.scattering = static_cast<float>(mo["scattering"].toDouble());
            data.surfaceMaterials.push_back(m);
        }
    }

    for (auto val : root["placedPoints"].toArray()) {
        QJsonObject po = val.toObject();
        PlacedPoint pt;
        pt.surfacePoint = jsonToVec3(po["surfacePoint"].toObject());
        pt.normal       = jsonToVec3(po["normal"].toObject());
        pt.distance     = static_cast<float>(po["distance"].toDouble());
        pt.color        = jsonToColor3f(po["color"].toArray());
        pt.pointType    = po["pointType"].toString().toStdString();
        pt.name         = po["name"].toString().toStdString();
        pt.volume       = static_cast<float>(po["volume"].toDouble(1.0));
        pt.audioFile    = po["audioFile"].toString().toStdString();
        pt.orientationYaw = static_cast<float>(po["orientationYaw"].toDouble(0.0));
        data.placedPoints.push_back(pt);
    }

    return data;
}

} // namespace prs
