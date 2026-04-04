#include "ProjectFile.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <cmath>

namespace prs {

namespace {

QJsonObject vec3ToJson(const Vec3f& v) {
    QJsonObject o;
    o["x"] = static_cast<double>(v.x());
    o["y"] = static_cast<double>(v.y());
    o["z"] = static_cast<double>(v.z());
    return o;
}

bool jsonToVec3(const QJsonValue& value, Vec3f* out) {
    if (!value.isObject())
        return false;

    const QJsonObject obj = value.toObject();
    if (!obj.contains("x") || !obj.contains("y") || !obj.contains("z"))
        return false;

    const double x = obj["x"].toDouble();
    const double y = obj["y"].toDouble();
    const double z = obj["z"].toDouble();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        return false;

    *out = Vec3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    return true;
}

QJsonArray color3fToJson(const Color3f& c) {
    QJsonArray a;
    a.append(static_cast<double>(c[0]));
    a.append(static_cast<double>(c[1]));
    a.append(static_cast<double>(c[2]));
    return a;
}

bool jsonToColor3f(const QJsonValue& value, Color3f* out) {
    if (!value.isArray())
        return false;

    const QJsonArray a = value.toArray();
    if (a.size() < 3)
        return false;

    const double r = a[0].toDouble();
    const double g = a[1].toDouble();
    const double b = a[2].toDouble();
    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b))
        return false;

    *out = {static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)};
    return true;
}

QJsonArray color3iToJson(const Color3i& c) {
    QJsonArray a;
    a.append(c[0]);
    a.append(c[1]);
    a.append(c[2]);
    return a;
}

bool jsonToColor3i(const QJsonValue& value, Color3i* out) {
    if (!value.isArray())
        return false;

    const QJsonArray a = value.toArray();
    if (a.size() < 3)
        return false;

    *out = {a[0].toInt(), a[1].toInt(), a[2].toInt()};
    return true;
}

QJsonObject materialToJson(const Material& m) {
    QJsonObject mo;
    mo["name"] = QString::fromStdString(m.name);
    mo["category"] = QString::fromStdString(m.category);
    mo["scattering"] = static_cast<double>(m.scattering);
    mo["texturePath"] = QString::fromStdString(m.texturePath);

    QJsonObject absObj;
    for (int i = 0; i < NUM_FREQ_BANDS; ++i)
        absObj[QString::number(FREQ_BANDS[i])] = static_cast<double>(m.absorption[i]);
    mo["absorption"] = absObj;
    mo["color"] = color3iToJson(m.color);
    mo["thickness"] = QString::fromStdString(m.thickness);
    return mo;
}

std::optional<Material> jsonToMaterial(const QJsonValue& value, QString* error) {
    if (!value.isObject()) {
        if (error)
            *error = "surfaceMaterials entries must be JSON objects or null";
        return std::nullopt;
    }

    const QJsonObject mo = value.toObject();
    Material m;
    m.name = mo["name"].toString().toStdString();
    m.category = mo["category"].toString().toStdString();
    m.scattering = static_cast<float>(mo["scattering"].toDouble(0.1));
    m.texturePath = mo["texturePath"].toString().toStdString();
    m.thickness = mo["thickness"].toString().toStdString();

    if (mo.contains("absorption")) {
        if (!mo["absorption"].isObject()) {
            if (error)
                *error = "material absorption must be an object";
            return std::nullopt;
        }

        const QJsonObject absObj = mo["absorption"].toObject();
        for (int i = 0; i < NUM_FREQ_BANDS; ++i) {
            const QString key = QString::number(FREQ_BANDS[i]);
            if (absObj.contains(key))
                m.absorption[i] = static_cast<float>(absObj[key].toDouble(0.2));
        }
    }

    if (mo.contains("color")) {
        if (!jsonToColor3i(mo["color"], &m.color)) {
            if (error)
                *error = "material color must be an RGB array";
            return std::nullopt;
        }
    }

    return m;
}

QJsonObject pointToJson(const PlacedPoint& pt) {
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
    return po;
}

std::optional<PlacedPoint> jsonToPlacedPoint(const QJsonValue& value, QString* error) {
    if (!value.isObject()) {
        if (error)
            *error = "placedPoints entries must be JSON objects";
        return std::nullopt;
    }

    const QJsonObject po = value.toObject();
    PlacedPoint pt;
    if (!jsonToVec3(po["surfacePoint"], &pt.surfacePoint)) {
        if (error)
            *error = "placed point surfacePoint must be an object with x/y/z";
        return std::nullopt;
    }
    if (!jsonToVec3(po["normal"], &pt.normal)) {
        if (error)
            *error = "placed point normal must be an object with x/y/z";
        return std::nullopt;
    }
    if (!jsonToColor3f(po["color"], &pt.color)) {
        if (error)
            *error = "placed point color must be an RGB array";
        return std::nullopt;
    }

    pt.distance = static_cast<float>(po["distance"].toDouble());
    pt.pointType = po["pointType"].toString().toStdString();
    pt.name = po["name"].toString().toStdString();
    pt.volume = static_cast<float>(po["volume"].toDouble(1.0));
    pt.audioFile = po["audioFile"].toString().toStdString();
    pt.orientationYaw = static_cast<float>(po["orientationYaw"].toDouble(0.0));
    return pt;
}

bool validateProjectData(const ProjectData& data, QString* error) {
    if (!std::isfinite(data.scaleFactor) || data.scaleFactor <= 0.0f) {
        if (error)
            *error = "scaleFactor must be a positive finite number";
        return false;
    }

    if (data.sampleRate <= 0) {
        if (error)
            *error = "sampleRate must be positive";
        return false;
    }

    if (!data.surfaceMaterials.empty() && data.surfaceMaterials.size() != data.surfaceColors.size()) {
        if (error)
            *error = "surfaceMaterials must be empty or match the surfaceColors count";
        return false;
    }

    for (const auto& c : data.surfaceColors) {
        for (float channel : c) {
            if (!std::isfinite(channel) || channel < 0.0f || channel > 1.0f) {
                if (error)
                    *error = "surfaceColors entries must be finite RGB values in the range [0, 1]";
                return false;
            }
        }
    }

    for (const auto& matOpt : data.surfaceMaterials) {
        if (!matOpt.has_value())
            continue;

        const Material& m = *matOpt;
        if (!std::isfinite(m.scattering) || m.scattering < 0.0f || m.scattering > 1.0f) {
            if (error)
                *error = "material scattering must be a finite value in the range [0, 1]";
            return false;
        }
        for (float a : m.absorption) {
            if (!std::isfinite(a) || a < 0.0f || a > 1.0f) {
                if (error)
                    *error = "material absorption must contain finite values in the range [0, 1]";
                return false;
            }
        }
    }

    for (const auto& pt : data.placedPoints) {
        if (!std::isfinite(pt.distance) || !std::isfinite(pt.volume) || !std::isfinite(pt.orientationYaw)) {
            if (error)
                *error = "placed point numeric values must be finite";
            return false;
        }
        if (pt.normal.squaredNorm() <= 0.0f) {
            if (error)
                *error = "placed point normal must be non-zero";
            return false;
        }
        if (pt.pointType != POINT_TYPE_NONE && pt.pointType != POINT_TYPE_SOURCE &&
            pt.pointType != POINT_TYPE_LISTENER) {
            if (error)
                *error = "placed point pointType must be none, source, or listener";
            return false;
        }
        if (pt.volume < 0.0f) {
            if (error)
                *error = "placed point volume must be non-negative";
            return false;
        }
        for (float channel : pt.color) {
            if (!std::isfinite(channel) || channel < 0.0f || channel > 1.0f) {
                if (error)
                    *error = "placed point color must contain finite RGB values in the range [0, 1]";
                return false;
            }
        }
    }

    return true;
}

std::optional<ProjectData> parseProjectObject(const QJsonObject& root, QString* error) {
    if (!root.contains("version") || !root["version"].isDouble()) {
        if (error)
            *error = "project file is missing version";
        return std::nullopt;
    }
    if (root["version"].toInt() != 1) {
        if (error)
            *error = "unsupported project file version";
        return std::nullopt;
    }

    if (!root.contains("stlFilePath") || !root["stlFilePath"].isString()) {
        if (error)
            *error = "project file is missing stlFilePath";
        return std::nullopt;
    }
    if (!root.contains("scaleFactor") || !root["scaleFactor"].isDouble()) {
        if (error)
            *error = "project file is missing scaleFactor";
        return std::nullopt;
    }
    if (!root.contains("soundSourceFile") || !root["soundSourceFile"].isString()) {
        if (error)
            *error = "project file is missing soundSourceFile";
        return std::nullopt;
    }
    if (!root.contains("surfaceColors") || !root["surfaceColors"].isArray()) {
        if (error)
            *error = "project file is missing surfaceColors";
        return std::nullopt;
    }
    if (!root.contains("surfaceMaterials") || !root["surfaceMaterials"].isArray()) {
        if (error)
            *error = "project file is missing surfaceMaterials";
        return std::nullopt;
    }
    if (!root.contains("placedPoints") || !root["placedPoints"].isArray()) {
        if (error)
            *error = "project file is missing placedPoints";
        return std::nullopt;
    }

    ProjectData data;
    data.stlFilePath = root["stlFilePath"].toString();
    data.scaleFactor = static_cast<float>(root["scaleFactor"].toDouble(1.0));
    data.soundSourceFile = root["soundSourceFile"].toString();
    data.sampleRate = root.contains("sampleRate") && root["sampleRate"].isDouble()
                          ? root["sampleRate"].toInt(DEFAULT_SAMPLE_RATE)
                          : DEFAULT_SAMPLE_RATE;

    if (root.contains("sampleRate") && (!root["sampleRate"].isDouble() || data.sampleRate <= 0)) {
        if (error)
            *error = "sampleRate must be a positive integer";
        return std::nullopt;
    }

    const QJsonArray colorsArr = root["surfaceColors"].toArray();
    for (const auto& val : colorsArr) {
        Color3f c;
        if (!jsonToColor3f(val, &c)) {
            if (error)
                *error = "surfaceColors entries must be RGB arrays";
            return std::nullopt;
        }
        data.surfaceColors.push_back(c);
    }

    const QJsonArray matsArr = root["surfaceMaterials"].toArray();
    for (const auto& val : matsArr) {
        if (val.isNull() || val.isUndefined()) {
            data.surfaceMaterials.push_back(std::nullopt);
            continue;
        }

        auto mat = jsonToMaterial(val, error);
        if (!mat.has_value())
            return std::nullopt;
        data.surfaceMaterials.push_back(std::move(*mat));
    }

    const QJsonArray pointsArr = root["placedPoints"].toArray();
    for (const auto& val : pointsArr) {
        auto pt = jsonToPlacedPoint(val, error);
        if (!pt.has_value())
            return std::nullopt;
        data.placedPoints.push_back(std::move(*pt));
    }

    if (!validateProjectData(data, error))
        return std::nullopt;

    return data;
}

QJsonObject projectDataToJson(const ProjectData& data) {
    QJsonObject root;
    root["version"] = 1;
    root["sampleRate"] = data.sampleRate;
    root["stlFilePath"] = data.stlFilePath;
    root["scaleFactor"] = static_cast<double>(data.scaleFactor);
    root["soundSourceFile"] = data.soundSourceFile;

    QJsonArray colorsArr;
    for (const auto& c : data.surfaceColors)
        colorsArr.append(color3fToJson(c));
    root["surfaceColors"] = colorsArr;

    QJsonArray matsArr;
    for (const auto& m : data.surfaceMaterials) {
        if (m.has_value())
            matsArr.append(materialToJson(*m));
        else
            matsArr.append(QJsonValue());
    }
    root["surfaceMaterials"] = matsArr;

    QJsonArray pointsArr;
    for (const auto& pt : data.placedPoints)
        pointsArr.append(pointToJson(pt));
    root["placedPoints"] = pointsArr;

    return root;
}

} // namespace

bool ProjectFile::save(const QString& filepath, const ProjectData& data) {
    QString error;
    if (!validateProjectData(data, &error)) {
        qWarning() << "Failed to save project:" << error;
        return false;
    }

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open project file for writing:" << filepath;
        return false;
    }

    const QJsonDocument doc(projectDataToJson(data));
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

std::optional<ProjectData> ProjectFile::load(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open project file for reading:" << filepath;
        return std::nullopt;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse project file:" << filepath << err.errorString();
        return std::nullopt;
    }

    QString error;
    auto data = parseProjectObject(doc.object(), &error);
    if (!data.has_value()) {
        qWarning() << "Failed to load project:" << filepath << error;
        return std::nullopt;
    }

    return data;
}

} // namespace prs
