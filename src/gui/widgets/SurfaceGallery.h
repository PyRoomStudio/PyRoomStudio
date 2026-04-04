#pragma once

#include "core/Types.h"
#include "gui/AssetsPanel.h"

#include <QGroupBox>
#include <QPixmap>
#include <QString>
#include <QVector>
#include <QWidget>

#include <vector>

namespace prs {

class ColorSwatch;

class SurfaceGallery : public QGroupBox {
    Q_OBJECT

  public:
    SurfaceGallery(const QString& title, const QVector<AssetsPanel::SurfaceInfo>& surfaces, QWidget* parent = nullptr);

    void updateColor(int surfaceIndex, const Color3i& color);
    void updateSurfaceAppearance(int surfaceIndex, const Color3i& color, const QPixmap& textureThumbnail);

  signals:
    void surfaceClicked(int surfaceIndex, const QString& name);

  private:
    struct SwatchEntry {
        int surfaceIndex;
        ColorSwatch* swatch;
    };
    std::vector<SwatchEntry> entries_;
};

} // namespace prs
