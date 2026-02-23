#pragma once

#include "rendering/Viewport3D.h"
#include "scene/SceneManager.h"
#include "core/PlacedPoint.h"

#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QSplitter>
#include <QUndoStack>
#include <QString>
#include <memory>
#include <optional>

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

    Viewport3D* viewport() const { return viewport_; }
    PropertyPanel* propertyPanel() const { return propertyPanel_; }
    LibraryPanel* libraryPanel() const { return libraryPanel_; }
    AssetsPanel* assetsPanel() const { return assetsPanel_; }
    BottomToolbar* bottomToolbar() const { return bottomToolbar_; }

private slots:
    // File menu
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
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

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupMenus();
    void setupToolbars();
    void setupCentralWidget();
    void connectSignals();
    void updateTitle();
    void populateAssetsFromRenderer(const QString& filepath);
    void saveProjectToFile(const QString& filepath);
    void addRecentProject(const QString& filepath);
    void updateRecentProjectsMenu();

    Viewport3D*    viewport_     = nullptr;
    LibraryPanel*  libraryPanel_ = nullptr;
    PropertyPanel* propertyPanel_ = nullptr;
    AssetsPanel*   assetsPanel_  = nullptr;
    BottomToolbar* bottomToolbar_ = nullptr;

    QToolBar* topToolbar_ = nullptr;

    // File menu actions
    QAction* actNewProject_  = nullptr;
    QAction* actOpenProject_ = nullptr;
    QAction* actSaveProject_ = nullptr;
    QAction* actSaveAsProject_ = nullptr;
    QAction* actExit_        = nullptr;
    QMenu*   recentProjectsMenu_ = nullptr;

    // Edit menu actions
    QAction* actUndo_      = nullptr;
    QAction* actRedo_      = nullptr;
    QAction* actCut_       = nullptr;
    QAction* actCopy_      = nullptr;
    QAction* actPaste_     = nullptr;
    QAction* actDelete_    = nullptr;
    QAction* actSelectAll_ = nullptr;

    // Settings menu
    QMenu*   settingsMenu_          = nullptr;
    QAction* actPreferences_        = nullptr;
    QAction* actDisplaySettings_    = nullptr;
    QAction* actAudioSettings_      = nullptr;
    QAction* actSimSettings_        = nullptr;
    QAction* actKeyboardShortcuts_  = nullptr;

    // Toolbar actions
    QAction* actToolMove_    = nullptr;
    QAction* actToolCopy_    = nullptr;
    QAction* actToolCut_     = nullptr;
    QAction* actToolPaste_   = nullptr;
    QAction* actToolDelete_  = nullptr;
    QAction* actToolMeasure_ = nullptr;

    // State
    QString soundSourceFile_;
    QString currentProjectFile_;
    bool    projectDirty_ = false;
    SceneManager sceneManager_;
    QUndoStack* undoStack_ = nullptr;
    std::optional<PlacedPoint> clipboardPoint_;
};

} // namespace prs
