#pragma once

#include <QString>

namespace prs {

QString resourcePath(const QString& relativePath);

/** Resolve a material JSON texture field to a path QFile/QImage can open. */
QString resolveMaterialTexturePath(const QString& storedPath);

} // namespace prs
