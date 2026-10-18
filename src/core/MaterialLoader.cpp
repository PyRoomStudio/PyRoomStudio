#include "MaterialLoader.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDirIterator>

namespace prs {

std::vector<MaterialCategory> MaterialLoader::loadFromDirectory(const QString& dirPath) {
    std::vector<MaterialCategory> categories;
    QDir root(dirPath);
    if (!root.exists()) return categories;

    // Walk nested folders; a category name is its path, e.g. "Wall Treatments/Curtains Fabrics".
    QStringList subDirs;
    QDirIterator it(dirPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        subDirs << root.relativeFilePath(it.fileInfo().absoluteFilePath());
    }
    subDirs.sort();

    for (const QString& sub : subDirs) {
        QDir catDir(root.filePath(sub));
        QStringList matFiles = catDir.entryList({"*.mat"}, QDir::Files, QDir::Name);
        if (matFiles.isEmpty()) continue;

        MaterialCategory category;
        // Convert each folder name to display name (e.g. "masonry_walls" -> "Masonry Walls")
        QStringList segments;
        for (QString displayName : sub.split('/')) {
            displayName.replace('_', ' ');
            QStringList words = displayName.split(' ', Qt::SkipEmptyParts);
            for (auto& w : words)
                if (!w.isEmpty()) w[0] = w[0].toUpper();
            segments << words.join(' ');
        }
        category.name = segments.join('/').toStdString();

        for (const QString& file : matFiles) {
            auto mat = loadFromFile(catDir.filePath(file));
            if (mat) {
                if (mat->category.empty())
                    mat->category = segments.first().toStdString();
                category.materials.push_back(std::move(*mat));
            }
        }

        if (!category.materials.empty())
            categories.push_back(std::move(category));
    }

    // Also load .mat files directly in the root (uncategorized)
    QStringList rootFiles = root.entryList({"*.mat"}, QDir::Files, QDir::Name);
    if (!rootFiles.isEmpty()) {
        MaterialCategory uncategorized;
        uncategorized.name = "Other";
        for (const QString& file : rootFiles) {
            auto mat = loadFromFile(root.filePath(file));
            if (mat) {
                if (mat->category.empty())
                    mat->category = "Other";
                uncategorized.materials.push_back(std::move(*mat));
            }
        }
        if (!uncategorized.materials.empty())
            categories.push_back(std::move(uncategorized));
    }

    return categories;
}

std::optional<Material> MaterialLoader::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return std::nullopt;

    QJsonObject obj = doc.object();
    Material mat;
    mat.name = obj["name"].toString().toStdString();
    mat.category = obj["category"].toString().toStdString();
    mat.thickness = obj["thickness"].toString().toStdString();
    mat.scattering = static_cast<float>(obj["scattering"].toDouble(0.1));
    mat.texturePath = obj["texture"].toString().toStdString();

    if (obj.contains("absorption") && obj["absorption"].isObject()) {
        QJsonObject abs = obj["absorption"].toObject();
        for (int i = 0; i < NUM_FREQ_BANDS; ++i) {
            QString key = QString::number(FREQ_BANDS[i]);
            if (abs.contains(key))
                mat.absorption[i] = static_cast<float>(abs[key].toDouble(0.2));
        }
    }

    if (obj.contains("color") && obj["color"].isArray()) {
        QJsonArray c = obj["color"].toArray();
        if (c.size() >= 3) {
            mat.color = {c[0].toInt(160), c[1].toInt(160), c[2].toInt(160)};
        }
    }

    return mat;
}

bool MaterialLoader::saveToFile(const QString& filePath, const Material& material) {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(material.name);
    obj["category"] = QString::fromStdString(material.category);
    obj["thickness"] = QString::fromStdString(material.thickness);
    obj["scattering"] = static_cast<double>(material.scattering);
    obj["texture"] = QString::fromStdString(material.texturePath);

    QJsonObject abs;
    for (int i = 0; i < NUM_FREQ_BANDS; ++i)
        abs[QString::number(FREQ_BANDS[i])] = static_cast<double>(material.absorption[i]);
    obj["absorption"] = abs;

    QJsonArray c;
    c.append(material.color[0]);
    c.append(material.color[1]);
    c.append(material.color[2]);
    obj["color"] = c;

    QJsonDocument doc(obj);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

} // namespace prs
