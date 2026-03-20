#pragma once

#include <QIcon>
#include <QString>

namespace prs {

/** Rasterize SVG from Qt resources (e.g. ":/toolbar/move.svg") for toolbars. Uses QSvgRenderer so
 *  the imageformats SVG plugin is not required (QIcon(":/x.svg") is often blank without it). */
QIcon iconFromSvgResource(const QString& qrcPath, int logicalSidePx = 32);

} // namespace prs
