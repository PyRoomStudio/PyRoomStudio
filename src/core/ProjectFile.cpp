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
    root["sampleRate"] = data.sampleRate;
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
            mo["category"] = QString::fromStdString(m->category);
            mo["scattering"] = static_cast<double>(m->scattering);
            mo["texturePath"] = QString::fromStdString(m->texturePath);
            QJsonObject absObj;
            for (int i = 0; i < NUM_FREQ_BANDS; ++i)
                absObj[QString::number(FREQ_BANDS[i])] = static_cast<double>(m->absorption[i]);
            mo["absorption"] = absObj;
            QJsonArray colorArr;
            colorArr.append(m->color[0]);
            colorArr.append(m->color[1]);
            colorArr.append(m->color[2]);
            mo["color"] = colorArr;
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
    data.sampleRate      = root.contains("sampleRate") ? root["sampleRate"].toInt(DEFAULT_SAMPLE_RATE) : DEFAULT_SAMPLE_RATE;
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
            m.category = mo["category"].toString().toStdString();
            m.scattering = static_cast<float>(mo["scattering"].toDouble(0.1));
            m.texturePath = mo["texturePath"].toString().toStdString();
            if (mo.contains("absorption") && mo["absorption"].isObject()) {
                QJsonObject absObj = mo["absorption"].toObject();
                for (int i = 0; i < NUM_FREQ_BANDS; ++i) {
                    QString key = QString::number(FREQ_BANDS[i]);
                    if (absObj.contains(key))
                        m.absorption[i] = static_cast<float>(absObj[key].toDouble(0.2));
                }
            }
            if (mo.contains("color") && mo["color"].isArray()) {
                QJsonArray ca = mo["color"].toArray();
                if (ca.size() >= 3)
                    m.color = {ca[0].toInt(160), ca[1].toInt(160), ca[2].toInt(160)};
            }
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
