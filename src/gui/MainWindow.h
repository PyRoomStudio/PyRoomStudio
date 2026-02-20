#pragma once

#include "rendering/Viewport3D.h"
#include "scene/SceneManager.h"

#include <QMainWindow>
#include <QAction>
#include <QToolBar>
#include <QSplitter>
#include <QString>
#include <memory>

namespace prs {

class LibraryPanel;
class PropertyPanel;
class AssetsPanel;
class BottomToolbar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // File menu
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onExit();

    // Bottom toolbar
    void onImportRoom();
    void onImportSound();
    void onPlacePoint();
    void onRender();

    // Viewport signals
    void onModelLoaded(const QString& filepath);
    void onPointSelected(int index);
    void onPointDeselected();
    void onSurfaceSelected(int index);
    void onSurfaceDeselected();
    void onPlacementModeChanged(bool enabled);
    void onScaleChanged(float factor);

private:
    void setupMenus();
    void setupToolbars();
    void setupCentralWidget();
    void connectSignals();
    void updateTitle();
    void populateAssetsFromRenderer(const QString& filepath);

    Viewport3D*    viewport_     = nullptr;
    LibraryPanel*  libraryPanel_ = nullptr;
    PropertyPanel* propertyPanel_ = nullptr;
    AssetsPanel*   assetsPanel_  = nullptr;
    BottomToolbar* bottomToolbar_ = nullptr;

    QToolBar* topToolbar_ = nullptr;

    // Actions
    QAction* actNewProject_  = nullptr;
    QAction* actOpenProject_ = nullptr;
    QAction* actSaveProject_ = nullptr;
    QAction* actExit_        = nullptr;

    // State
    QString soundSourceFile_;
    SceneManager sceneManager_;
};

} // namespace prs
