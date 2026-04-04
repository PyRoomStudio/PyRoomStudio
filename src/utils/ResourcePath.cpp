#include "ResourcePath.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

namespace prs {

QString resourcePath(const QString& relativePath) {
    // Qt resource system works on all platforms including WebAssembly.
    QString qrcPath = ":/" + relativePath;
    if (QFile::exists(qrcPath))
        return qrcPath;

#ifndef SEICHE_WEB_BUILD
    // Filesystem fallbacks are only meaningful on desktop. In the browser
    // there is no accessible application directory or working directory.

    // Try relative to the executable
    QString appDir = QCoreApplication::applicationDirPath();
    QString fullPath = QDir(appDir).filePath(relativePath);
    if (QFile::exists(fullPath))
        return fullPath;

    // Fallback: relative to current directory
    return relativePath;
#else
    // In the browser build all resources must be bundled in the Qt resource
    // system.  Return an empty string to signal that the resource was not found
    // so callers can handle the missing-resource case gracefully.
    return {};
#endif
}

QString resolveMaterialTexturePath(const QString& storedPath) {
    if (storedPath.isEmpty())
        return {};

#ifndef SEICHE_WEB_BUILD
    // On desktop, absolute stored paths may point directly to a file.
    if (QFile::exists(storedPath))
        return storedPath;
#endif

    QString viaResource = resourcePath(storedPath);
    if (!viaResource.isEmpty() && QFile::exists(viaResource))
        return viaResource;

#ifndef SEICHE_WEB_BUILD
    // On desktop, return the original stored path even when it cannot be found
    // so callers can display it or handle the missing-resource case themselves.
    return storedPath;
#else
    // In the browser there is no local filesystem, so a raw path that didn't
    // resolve via the Qt resource system is unusable.  Return empty so callers
    // can detect and handle the missing-resource case gracefully.
    return {};
#endif
}

} // namespace prs
