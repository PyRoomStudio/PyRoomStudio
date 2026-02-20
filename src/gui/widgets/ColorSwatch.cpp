#include "ColorSwatch.h"

#include <QPainter>
#include <QMouseEvent>

namespace prs {

ColorSwatch::ColorSwatch(const QString& label, const Color3i& color, QWidget* parent)
    : QWidget(parent), label_(label), color_(color)
{
    setMinimumSize(60, 60);
    setCursor(Qt::PointingHandCursor);
}

void ColorSwatch::setColor(const Color3i& color) {
    color_ = color;
    update();
}

void ColorSwatch::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = rect().adjusted(1, 1, -1, -1);
    int colorHeight = r.height() - 20;

    // Color rectangle
    QRect colorRect(r.x(), r.y(), r.width(), colorHeight);
    QColor qcolor(color_[0], color_[1], color_[2]);
    p.fillRect(colorRect, qcolor);

    // Border
    p.setPen(hovered_ ? QColor(70, 130, 180) : QColor(64, 64, 64));
    p.drawRect(colorRect);

    // Label
    QFont font = p.font();
    font.setPointSize(8);
    p.setFont(font);
    p.setPen(Qt::black);
    QRect textRect(r.x(), r.bottom() - 18, r.width(), 18);
    p.drawText(textRect, Qt::AlignCenter, label_);
}

void ColorSwatch::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        emit clicked();
    QWidget::mousePressEvent(event);
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
