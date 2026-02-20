#pragma once

#include "core/Types.h"

#include <QWidget>
#include <QString>

namespace prs {

class ColorSwatch : public QWidget {
    Q_OBJECT

public:
    ColorSwatch(const QString& label, const Color3i& color,
                QWidget* parent = nullptr);

    void setColor(const Color3i& color);
    QSize sizeHint() const override { return {70, 70}; }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QString label_;
    Color3i color_;
    bool hovered_ = false;
};

} // namespace prs
