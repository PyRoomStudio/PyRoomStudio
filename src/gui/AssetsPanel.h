#pragma once

#include "core/Types.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QString>
#include <QVector>

namespace prs {

class SurfaceGallery;

class AssetsPanel : public QWidget {
    Q_OBJECT

public:
    explicit AssetsPanel(QWidget* parent = nullptr);

    struct SurfaceInfo {
        QString name;
        Color3i color;
        int index;
    };

    void addStlSurfaces(const QString& stlName, const QVector<SurfaceInfo>& surfaces);
    void updateSurfaceColor(int surfaceIndex, const Color3i& color);
    void clearSurfaces();

signals:
    void surfaceClicked(int surfaceIndex, const QString& name);

private:
    void setupUI();

    QVBoxLayout*    contentLayout_ = nullptr;
    SurfaceGallery* gallery_       = nullptr;
};

} // namespace prs
