#include "ResourcePath.h"
#include <QCoreApplication>
#include <QDir>

namespace prs {

QString resourcePath(const QString& relativePath) {
    // First try Qt resource system
    QString qrcPath = ":/" + relativePath;
    if (QFile::exists(qrcPath)) return qrcPath;

    // Then try relative to the executable
    QString appDir = QCoreApplication::applicationDirPath();
    QString fullPath = QDir(appDir).filePath(relativePath);
    if (QFile::exists(fullPath)) return fullPath;

    // Fallback: relative to current directory
    return relativePath;
}

} // namespace prs
