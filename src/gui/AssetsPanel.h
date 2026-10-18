#pragma once

#include "core/PlacedPoint.h"

#include <QWidget>
#include <QString>
#include <QVector>
#include <QJsonArray>
#include <vector>

class QTabWidget;
class QListWidget;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace prs {

// Surface groups live in the user's profile file, keyed by absolute model path,
// so every project that opens the same model shares them.
QJsonArray loadSurfaceGroups(const QString& modelPath);
bool saveSurfaceGroups(const QString& modelPath, const QJsonArray& tree);

class AssetsPanel : public QWidget {
    Q_OBJECT

public:
    explicit AssetsPanel(QWidget* parent = nullptr);

    struct SurfaceInfo {
        QString name;
        int index;
    };

    void addStlSurfaces(const QString& stlName, const QVector<SurfaceInfo>& surfaces);
    void clearSurfaces();
    void updatePointLists(const std::vector<PlacedPoint>& points, int activePointIndex);

    // Children of the model item: surface index (int) or {"name": str, "children": [...]}.
    QJsonArray surfaceTree() const;
    // Rebuilds groups from untrusted JSON; bad entries are skipped and
    // surfaces it does not place are appended to the model item.
    void restoreSurfaceTree(const QJsonArray& tree);

signals:
    void surfaceClicked(int surfaceIndex, const QString& name);
    void pointListClicked(int pointIndex);
    void surfaceTreeChanged(); // user created, renamed, deleted or moved a group

private:
    void setupUI();
    void applyFilter();
    void showSurfaceMenu(const QPoint& pos);
    QTreeWidgetItem* addGroup(QTreeWidgetItem* parent, const QString& name);

    QTreeWidget*     surfaceTree_   = nullptr;
    QTreeWidgetItem* modelItem_     = nullptr;
    QTabWidget*      tabWidget_     = nullptr;
    QLineEdit*       search_        = nullptr;
    QListWidget*     sourcesList_   = nullptr;
    QListWidget*     listenersList_ = nullptr;
};

} // namespace prs
