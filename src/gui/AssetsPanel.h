#pragma once

#include "core/PlacedPoint.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QString>
#include <QVector>
#include <QPixmap>
#include <vector>

class QTabWidget;
class QListWidget;

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
        QString texturePath;
        QPixmap textureThumbnail;
    };

    void addStlSurfaces(const QString& stlName, const QVector<SurfaceInfo>& surfaces);
    void updateSurfaceColor(int surfaceIndex, const Color3i& color);
    void updateSurfaceAppearance(int surfaceIndex, const Color3i& color, const QPixmap& textureThumbnail);
    void clearSurfaces();
    void updatePointLists(const std::vector<PlacedPoint>& points, int activePointIndex);

signals:
    void surfaceClicked(int surfaceIndex, const QString& name);
    void pointListClicked(int pointIndex);

private:
    void setupUI();

    QVBoxLayout*    contentLayout_ = nullptr;
    SurfaceGallery* gallery_       = nullptr;
    QTabWidget*     tabWidget_     = nullptr;
    QWidget*        surfacesTab_   = nullptr;
    QListWidget*    sourcesList_   = nullptr;
    QListWidget*    listenersList_ = nullptr;
};

} // namespace prs
