#pragma once

#include "core/Types.h"

#include <QWidget>
#include <QString>
#include <QPoint>
#include <QPixmap>
#include <vector>

class QGridLayout;

namespace prs {

class ColorSwatch : public QWidget {
    Q_OBJECT

public:
    ColorSwatch(const QString& label, const Color3i& color,
                QWidget* parent = nullptr);

    void setColor(const Color3i& color);
    void setSurfaceAppearance(const Color3i& color, const QPixmap& texturePreview);
    void setPreviewPixmap(const QPixmap& pixmap);
    void clearPreviewPixmap();
    void setDragData(const QByteArray& data);
    const QString& label() const { return label_; }
    QSize sizeHint() const override { return {70, 70}; }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QString label_;
    Color3i color_;
    QPixmap previewPixmap_;
    bool hovered_ = false;
    QPoint dragStartPos_;
    QByteArray dragData_;
};

// Shows swatches whose label contains `filter` (case-insensitive), packed into
// `grid` without gaps. Returns the number shown.
int filterSwatchGrid(QGridLayout* grid, const std::vector<ColorSwatch*>& swatches,
                     const QString& filter, int cols = 2);

} // namespace prs
