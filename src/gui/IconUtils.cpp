#include "IconUtils.h"

#include <QByteArray>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

namespace prs {

namespace {

/** Figma-style SVGs wrap a PNG in data-uri; QSvgRenderer often draws nothing for those. */
static QImage decodeEmbeddedPngFromSvg(const QByteArray& svgXml) {
    static const QByteArray needle = QByteArrayLiteral("data:image/png;base64,");
    int idx = svgXml.indexOf(needle);
    if (idx < 0)
        return {};
    idx += needle.size();
    int end = idx;
    while (end < svgXml.size() && svgXml[end] != '"')
        ++end;
    QByteArray raw = QByteArray::fromBase64(svgXml.mid(idx, end - idx));
    if (raw.isEmpty())
        return {};
    QImage img;
    if (!img.loadFromData(raw, "PNG"))
        return {};
    return img;
}

static QIcon iconFromSquareImage(const QImage& src, int side) {
    if (src.isNull())
        return {};

    QImage out(side, side, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);

    const QImage scaled = src.scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter p(&out);
    const int x = (side - scaled.width()) / 2;
    const int y = (side - scaled.height()) / 2;
    p.drawImage(x, y, scaled);
    p.end();

    return QIcon(QPixmap::fromImage(out));
}

} // namespace

QIcon iconFromSvgResource(const QString& qrcPath, int logicalSidePx) {
    QFile f(qrcPath);
    if (!f.open(QIODevice::ReadOnly))
        return {};

    const QByteArray svgData = f.readAll();

    // Prefer embedded bitmap (matches current toolbar asset format).
    QImage embedded = decodeEmbeddedPngFromSvg(svgData);
    if (!embedded.isNull())
        return iconFromSquareImage(embedded, logicalSidePx);

    QSvgRenderer renderer(svgData);
    if (!renderer.isValid())
        return {};

    QImage img(logicalSidePx, logicalSidePx, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    renderer.render(&p, QRectF(0, 0, logicalSidePx, logicalSidePx));
    p.end();

    return QIcon(QPixmap::fromImage(img));
}

} // namespace prs
