#include "ResourcePath.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

namespace prs {

QString resourcePath(const QString& relativePath) {
    // First try Qt resource system
    QString qrcPath = ":/" + relativePath;
    if (QFile::exists(qrcPath))
        return qrcPath;

    // Then try relative to the executable
    QString appDir = QCoreApplication::applicationDirPath();
    QString fullPath = QDir(appDir).filePath(relativePath);
    if (QFile::exists(fullPath))
        return fullPath;

    // Fallback: relative to current directory
    return relativePath;
}

QString resolveMaterialTexturePath(const QString& storedPath) {
    if (storedPath.isEmpty())
        return {};
    if (QFile::exists(storedPath))
        return storedPath;
    QString viaResource = resourcePath(storedPath);
    if (QFile::exists(viaResource))
        return viaResource;
    return storedPath;
}

} // namespace prs
