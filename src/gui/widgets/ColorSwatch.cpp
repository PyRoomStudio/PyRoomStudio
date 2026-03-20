#include "ColorSwatch.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

namespace prs {

ColorSwatch::ColorSwatch(const QString& label, const Color3i& color, QWidget* parent)
    : QWidget(parent), label_(label), color_(color)
{
    setMinimumSize(60, 60);
    setCursor(Qt::PointingHandCursor);
}

void ColorSwatch::setColor(const Color3i& color) {
    color_ = color;
    previewPixmap_ = QPixmap();
    update();
}

void ColorSwatch::setSurfaceAppearance(const Color3i& color, const QPixmap& texturePreview) {
    color_ = color;
    previewPixmap_ = texturePreview;
    update();
}

void ColorSwatch::setPreviewPixmap(const QPixmap& pixmap) {
    previewPixmap_ = pixmap;
    update();
}

void ColorSwatch::clearPreviewPixmap() {
    previewPixmap_ = QPixmap();
    update();
}

void ColorSwatch::setDragData(const QByteArray& data) {
    dragData_ = data;
}

void ColorSwatch::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = rect().adjusted(1, 1, -1, -1);
    int colorHeight = r.height() - 20;

    QRect colorRect(r.x(), r.y(), r.width(), colorHeight);
    QColor qcolor(color_[0], color_[1], color_[2]);
    if (!previewPixmap_.isNull()) {
        p.fillRect(colorRect, qcolor);
        QPixmap scaled = previewPixmap_.scaled(colorRect.size(), Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);
        int x = colorRect.x() + (colorRect.width() - scaled.width()) / 2;
        int y = colorRect.y() + (colorRect.height() - scaled.height()) / 2;
        p.drawPixmap(x, y, scaled);
    } else {
        p.fillRect(colorRect, qcolor);
    }

    p.setPen(hovered_ ? QColor(70, 130, 180) : QColor(64, 64, 64));
    p.drawRect(colorRect);

    QFont font = p.font();
    font.setPointSize(8);
    p.setFont(font);
    p.setPen(Qt::black);
    QRect textRect(r.x(), r.bottom() - 18, r.width(), 18);
    p.drawText(textRect, Qt::AlignCenter, label_);
}

void ColorSwatch::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragStartPos_ = event->pos();
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void ColorSwatch::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if (dragData_.isEmpty())
        return;
    if ((event->pos() - dragStartPos_).manhattanLength() < QApplication::startDragDistance())
        return;

    auto* drag = new QDrag(this);
    auto* mimeData = new QMimeData;
    mimeData->setData("application/x-prs-material", dragData_);
    drag->setMimeData(mimeData);

    QPixmap pixmap(40, 40);
    if (!previewPixmap_.isNull())
        pixmap = previewPixmap_.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else
        pixmap.fill(QColor(color_[0], color_[1], color_[2]));
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(20, 20));

    drag->exec(Qt::CopyAction);
}

void ColorSwatch::enterEvent(QEnterEvent*) {
    hovered_ = true;
    update();
}

void ColorSwatch::leaveEvent(QEvent*) {
    hovered_ = false;
    update();
}

} // namespace prs
