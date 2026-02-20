#pragma once

#include "core/Types.h"
#include "gui/AssetsPanel.h"

#include <QWidget>
#include <QGroupBox>
#include <QString>
#include <QVector>
#include <vector>

namespace prs {

class ColorSwatch;

class SurfaceGallery : public QGroupBox {
    Q_OBJECT

public:
    SurfaceGallery(const QString& title,
                   const QVector<AssetsPanel::SurfaceInfo>& surfaces,
                   QWidget* parent = nullptr);

    void updateColor(int surfaceIndex, const Color3i& color);

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
