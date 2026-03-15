#include "MainWindow.h"
#include "UndoCommands.h"
#include "LibraryPanel.h"
#include "PropertyPanel.h"
#include "AssetsPanel.h"
#include "BottomToolbar.h"
#include "acoustics/AcousticSimulator.h"
#include "acoustics/SimulationWorker.h"
#include "acoustics/SimulationQueue.h"
#include "SimulationQueuePanel.h"
#include "core/ProjectFile.h"
#include "dialogs/SettingsDialogs.h"
#include "dialogs/AudioComparisonDialog.h"
#include "dialogs/RenderOptionsDialog.h"

#include <QMenuBar>
#include <QToolBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStatusBar>
#include <QApplication>
#include <QCloseEvent>
#include <QSettings>
#include <QThread>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QUrl>

namespace prs {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("PyRoomStudio");
    resize(1200, 800);

    undoStack_ = new QUndoStack(this);
    connect(undoStack_, &QUndoStack::cleanChanged, [this](bool) {
        projectDirty_ = !undoStack_->isClean();
        updateTitle();
    });

    autoSaveTimer_ = new QTimer(this);
    connect(autoSaveTimer_, &QTimer::timeout, this, [this]() {
        if (projectDirty_ && !currentProjectFile_.isEmpty()) {
            saveProjectToFile(currentProjectFile_);
            statusBar()->showMessage("Auto-saved", 3000);
        }
    });

    simQueue_ = new SimulationQueue(this);

    setupMenus();
    setupToolbars();
    setupCentralWidget();
    connectSignals();
    updateRecentProjectsMenu();
    configureAutoSaveTimer();

    statusBar()->showMessage("Ready");
}

void MainWindow::setupMenus() {
    auto* fileMenu = menuBar()->addMenu("&File");
    actNewProject_    = fileMenu->addAction("&New Project", QKeySequence::New, this, &MainWindow::onNewProject);
    actOpenProject_   = fileMenu->addAction("&Open Project...", QKeySequence::Open, this, &MainWindow::onOpenProject);
    actSaveProject_   = fileMenu->addAction("&Save Project", QKeySequence::Save, this, &MainWindow::onSaveProject);
    actSaveAsProject_ = fileMenu->addAction("Save Project &As...", QKeySequence("Ctrl+Shift+S"), this, &MainWindow::onSaveProjectAs);
    fileMenu->addSeparator();
    recentProjectsMenu_ = fileMenu->addMenu("Recent Projects");
    recentProjectsMenu_->addAction("(empty)")->setEnabled(false);
    fileMenu->addSeparator();
    actExit_ = fileMenu->addAction("E&xit", QKeySequence::Quit, this, &MainWindow::onExit);

    auto* editMenu = menuBar()->addMenu("&Edit");
    actUndo_ = undoStack_->createUndoAction(this, "&Undo");
    actUndo_->setShortcut(QKeySequence::Undo);
    editMenu->addAction(actUndo_);
    actRedo_ = undoStack_->createRedoAction(this, "&Redo");
    actRedo_->setShortcut(QKeySequence("Ctrl+Y"));
    editMenu->addAction(actRedo_);
    editMenu->addSeparator();
    actCut_ = editMenu->addAction("Cu&t", QKeySequence::Cut, this, [this]() {
        if (viewport_->activePointIndex() >= 0) {
            clipboardPoint_ = viewport_->placedPoints()[viewport_->activePointIndex()];
            undoStack_->push(new RemovePointCommand(viewport_, viewport_->activePointIndex()));
        }
    });
    actCopy_ = editMenu->addAction("&Copy", QKeySequence::Copy, this, [this]() {
        if (viewport_->activePointIndex() >= 0)
            clipboardPoint_ = viewport_->placedPoints()[viewport_->activePointIndex()];
    });
    actPaste_ = editMenu->addAction("&Paste", QKeySequence::Paste, this, [this]() {
        if (clipboardPoint_.has_value())
            undoStack_->push(new AddPointCommand(viewport_, *clipboardPoint_));
    });
    actDelete_ = editMenu->addAction("&Delete", QKeySequence::Delete, this, [this]() {
        auto& sel = viewport_->selectedPointIndices();
        if (!sel.empty()) {
            undoStack_->beginMacro("Delete Selected Points");
            std::vector<int> sorted(sel.rbegin(), sel.rend());
            for (int idx : sorted)
                undoStack_->push(new RemovePointCommand(viewport_, idx));
            undoStack_->endMacro();
            viewport_->clearPointSelection();
        } else if (viewport_->activePointIndex() >= 0) {
            undoStack_->push(new RemovePointCommand(viewport_, viewport_->activePointIndex()));
        }
    });
    editMenu->addSeparator();
    actSelectAll_ = editMenu->addAction("Select &All", QKeySequence::SelectAll, this, [this]() {
        if (viewport_->hasModel() && !viewport_->placedPoints().empty()) {
            viewport_->selectAllPoints();
        }
    });

    settingsMenu_ = menuBar()->addMenu("&Settings");
    actPreferences_ = settingsMenu_->addAction("&Preferences...", this, [this]() {
        PreferencesDialog dlg(this);
        dlg.exec();
        configureAutoSaveTimer();
    });
    actDisplaySettings_ = settingsMenu_->addAction("&Display Settings...", this, [this]() {
        DisplaySettingsDialog dlg(this);
        connect(&dlg, &DisplaySettingsDialog::settingsChanged, this, [this]() {
            viewport_->applyDisplaySettings();
        });
        dlg.exec();
    });
    actAudioSettings_ = settingsMenu_->addAction("&Audio Settings...", this, [this]() {
        AudioSettingsDialog dlg(this);
        dlg.exec();
    });
    actSimSettings_ = settingsMenu_->addAction("Si&mulation Settings...", this, [this]() {
        SimulationSettingsDialog dlg(this);
        dlg.exec();
    });
    settingsMenu_->addSeparator();
    actKeyboardShortcuts_ = settingsMenu_->addAction("&Keyboard Shortcuts...", this, [this]() {
        KeyboardShortcutsDialog dlg(this);
        dlg.exec();
    });
}

void MainWindow::setupToolbars() {
    topToolbar_ = addToolBar("Tools");
    topToolbar_->setMovable(false);

    actToolMove_ = topToolbar_->addAction("Move");
    actToolMove_->setCheckable(true);
    actToolMove_->setToolTip("Drag placed points to new positions");
    connect(actToolMove_, &QAction::toggled, [this](bool checked) {
        viewport_->setMoveMode(checked);
    });

    actToolCopy_ = topToolbar_->addAction("Copy");
    connect(actToolCopy_, &QAction::triggered, [this]() {
        if (viewport_->activePointIndex() >= 0)
            clipboardPoint_ = viewport_->placedPoints()[viewport_->activePointIndex()];
    });

    actToolCut_ = topToolbar_->addAction("Cut");
    connect(actToolCut_, &QAction::triggered, [this]() {
        if (viewport_->activePointIndex() >= 0) {
            clipboardPoint_ = viewport_->placedPoints()[viewport_->activePointIndex()];
            undoStack_->push(new RemovePointCommand(viewport_, viewport_->activePointIndex()));
        }
    });

    actToolPaste_ = topToolbar_->addAction("Paste");
    connect(actToolPaste_, &QAction::triggered, [this]() {
        if (clipboardPoint_.has_value())
            undoStack_->push(new AddPointCommand(viewport_, *clipboardPoint_));
    });

    actToolDelete_ = topToolbar_->addAction("Delete");
    connect(actToolDelete_, &QAction::triggered, [this]() {
        if (viewport_->activePointIndex() >= 0)
            undoStack_->push(new RemovePointCommand(viewport_, viewport_->activePointIndex()));
    });

    actToolMeasure_ = topToolbar_->addAction("Measure");
    actToolMeasure_->setCheckable(true);
    connect(actToolMeasure_, &QAction::toggled, [this](bool checked) {
        viewport_->setMeasureMode(checked);
    });
}

void MainWindow::setupCentralWidget() {
    auto* centralWidget = new QWidget;
    auto* mainLayout    = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: Library panel
    libraryPanel_ = new LibraryPanel;
    libraryPanel_->setFixedWidth(200);
    splitter->addWidget(libraryPanel_);

    // Center: 3D viewport
    viewport_ = new Viewport3D;
    splitter->addWidget(viewport_);

    // Right: stacked property + assets
    auto* rightWidget = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);

    propertyPanel_ = new PropertyPanel;
    assetsPanel_   = new AssetsPanel;

    rightLayout->addWidget(propertyPanel_);
    rightLayout->addWidget(assetsPanel_);
    rightWidget->setFixedWidth(210);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);

    mainLayout->addWidget(splitter, 1);

    // Simulation queue panel
    simQueuePanel_ = new SimulationQueuePanel(simQueue_);
    simQueuePanel_->setMaximumHeight(180);
    mainLayout->addWidget(simQueuePanel_);

    // Bottom toolbar
    bottomToolbar_ = new BottomToolbar;
    mainLayout->addWidget(bottomToolbar_);

    setCentralWidget(centralWidget);
}

void MainWindow::connectSignals() {
    // Viewport signals
    connect(viewport_, &Viewport3D::modelLoaded,           this, &MainWindow::onModelLoaded);
    connect(viewport_, &Viewport3D::meshOpenWarning, this, [this](int boundaryEdges) {
        QMessageBox::warning(this, "Open Mesh Detected",
            QString("The loaded mesh has %1 non-manifold/boundary edge(s) and is not watertight.\n\n"
                    "Acoustic simulation results may be inaccurate for open meshes.")
            .arg(boundaryEdges));
    });
    connect(viewport_, &Viewport3D::pointPlaced,           this, &MainWindow::onPointSelected);
    connect(viewport_, &Viewport3D::pointSelected,         this, &MainWindow::onPointSelected);
    connect(viewport_, &Viewport3D::pointDeselected,       this, &MainWindow::onPointDeselected);
    connect(viewport_, &Viewport3D::surfaceSelected,       this, &MainWindow::onSurfaceSelected);
    connect(viewport_, &Viewport3D::surfaceDeselected,     this, &MainWindow::onSurfaceDeselected);
    connect(viewport_, &Viewport3D::placementModeChanged,  this, &MainWindow::onPlacementModeChanged);
    connect(viewport_, &Viewport3D::scaleChanged,          this, &MainWindow::onScaleChanged);

    connect(viewport_, &Viewport3D::measurementResult, [this](float dist) {
        statusBar()->showMessage(QString("Distance: %1 m").arg(dist, 0, 'f', 3));
    });

    connect(viewport_, &Viewport3D::moveFinished,
        [this](int index, const PlacedPoint& oldState, const PlacedPoint& newState) {
            undoStack_->push(new MovePointCommand(viewport_, index, oldState, newState));
        });

    // Assets panel -> viewport (surface selection from gallery click)
    connect(assetsPanel_, &AssetsPanel::surfaceClicked, [this](int surfIdx, const QString&) {
        viewport_->selectSurface(surfIdx);
    });

    // Bottom toolbar
    connect(bottomToolbar_, &BottomToolbar::importRoomClicked,  this, &MainWindow::onImportRoom);
    connect(bottomToolbar_, &BottomToolbar::importSoundClicked, this, &MainWindow::onImportSound);
    connect(bottomToolbar_, &BottomToolbar::placePointClicked,  this, &MainWindow::onPlacePoint);
    connect(bottomToolbar_, &BottomToolbar::renderClicked,      this, &MainWindow::onRender);

    // Simulation queue
    connect(simQueue_, &SimulationQueue::jobFinished, this, [this](int, const QString& outputDir) {
        updateTitle();
        auto answer = QMessageBox::question(this, "Simulation Complete",
            "Output saved to:\n" + outputDir + "\n\nCompare audio files?",
            QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            auto* dlg = new AudioComparisonDialog(outputDir, this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->show();
        }
        statusBar()->showMessage("Simulation complete: " + outputDir);
    });
    connect(simQueuePanel_, &SimulationQueuePanel::openResults, this, [this](const QString& outputDir) {
        auto* dlg = new AudioComparisonDialog(outputDir, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    });

    connect(simQueue_, &SimulationQueue::jobError, this, [this](int, const QString& msg) {
        updateTitle();
        if (msg != "Cancelled")
            QMessageBox::warning(this, "Simulation Failed", msg);
        else
            statusBar()->showMessage("Simulation cancelled");
    });

    // Property panel -> viewport
    connect(propertyPanel_, &PropertyPanel::scaleChanged, [this](float v) {
        viewport_->setScaleFactor(v);
    });
    connect(propertyPanel_, &PropertyPanel::pointDistanceChanged, [this](float v) {
        viewport_->updateActivePointDistance(v);
    });
    connect(propertyPanel_, &PropertyPanel::setPointSource, [this]() {
        viewport_->setActivePointType(POINT_TYPE_SOURCE);
        propertyPanel_->setOrientationControlsVisible(false);
        viewport_->update();
    });
    connect(propertyPanel_, &PropertyPanel::setPointListener, [this]() {
        viewport_->setActivePointType(POINT_TYPE_LISTENER);
        propertyPanel_->setOrientationControlsVisible(true);
        int idx = viewport_->activePointIndex();
        if (idx >= 0 && idx < static_cast<int>(viewport_->placedPoints().size()))
            propertyPanel_->setPointOrientationYaw(viewport_->placedPoints()[idx].orientationYaw);
        viewport_->update();
    });
    connect(propertyPanel_, &PropertyPanel::pointOrientationYawChanged, [this](float yaw) {
        int idx = viewport_->activePointIndex();
        if (idx >= 0 && idx < static_cast<int>(viewport_->placedPoints().size())) {
            viewport_->placedPoints()[idx].orientationYaw = yaw;
            viewport_->update();
        }
    });
    connect(propertyPanel_, &PropertyPanel::deletePoint, [this]() {
        viewport_->removeActivePoint();
    });
    connect(propertyPanel_, &PropertyPanel::deselectPoint, [this]() {
        viewport_->deselectPoint();
    });
    connect(propertyPanel_, &PropertyPanel::toggleTexture, [this]() {
        int si = viewport_->selectedSurfaceIndex();
        if (si >= 0) viewport_->toggleSurfaceTexture(si);
    });
    connect(propertyPanel_, &PropertyPanel::loadTexture, [this]() {
        QString path = QFileDialog::getOpenFileName(this,
            "Load Texture Image", QString(),
            "Images (*.png *.jpg *.jpeg *.bmp);;All files (*.*)");
        if (!path.isEmpty()) {
            if (viewport_->loadTexture(path))
                statusBar()->showMessage("Texture loaded: " + QFileInfo(path).fileName());
            else
                QMessageBox::warning(this, "Error", "Failed to load texture image.");
        }
    });
    connect(propertyPanel_, &PropertyPanel::deselectSurface, [this]() {
        viewport_->deselectSurface();
    });
    connect(propertyPanel_, &PropertyPanel::pointNameChanged, [this](const QString& name) {
        int idx = viewport_->activePointIndex();
        if (idx >= 0 && idx < static_cast<int>(viewport_->placedPoints().size()))
            viewport_->placedPoints()[idx].name = name.toStdString();
    });
    connect(propertyPanel_, &PropertyPanel::pointVolumeChanged, [this](float vol) {
        int idx = viewport_->activePointIndex();
        if (idx >= 0 && idx < static_cast<int>(viewport_->placedPoints().size()))
            viewport_->placedPoints()[idx].volume = vol;
    });
    connect(propertyPanel_, &PropertyPanel::selectPointAudioFile, [this]() {
        int idx = viewport_->activePointIndex();
        if (idx < 0) return;
        QString path = QFileDialog::getOpenFileName(this,
            "Select Audio File", defaultProjectDir(),
            "Audio files (*.wav *.mp3 *.flac *.ogg);;All files (*.*)");
        if (path.isEmpty()) return;
        viewport_->placedPoints()[idx].audioFile = path.toStdString();
        propertyPanel_->setPointAudioFile(QFileInfo(path).fileName());
    });

    // Surface material change -> PropertyPanel update
    connect(viewport_, &Viewport3D::surfaceMaterialChanged,
        [this](int, const QString& materialName) {
            propertyPanel_->setMaterialName(materialName);
        });

    // Library panel -> sound file selection
    connect(libraryPanel_, &LibraryPanel::soundFileSelected, [this](const QString& path) {
        int idx = viewport_->activePointIndex();
        if (idx >= 0 && viewport_->placedPoints()[idx].pointType == POINT_TYPE_SOURCE) {
            viewport_->placedPoints()[idx].audioFile = path.toStdString();
            propertyPanel_->setPointAudioFile(QFileInfo(path).fileName());
            statusBar()->showMessage("Audio assigned to source: " + QFileInfo(path).fileName());
        } else {
            soundSourceFile_ = path;
            updateTitle();
            statusBar()->showMessage("Sound loaded: " + path);
        }
    });

    // Library panel -> viewport (material selection)
    connect(libraryPanel_, &LibraryPanel::materialSelected,
        [this](const QString& name, const Color3f& color, float absorption) {
            int si = viewport_->selectedSurfaceIndex();
            if (si >= 0) {
                Material mat;
                mat.name = name.toStdString();
                mat.color = {static_cast<int>(color[0] * 255),
                             static_cast<int>(color[1] * 255),
                             static_cast<int>(color[2] * 255)};
                mat.energyAbsorption = absorption;
                viewport_->assignMaterial(si, mat);
            }
        });
}

// ==================== Slots ====================

void MainWindow::onNewProject() {
    if (projectDirty_) {
        auto res = QMessageBox::question(this, "Unsaved Changes",
            "Save current project before creating a new one?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (res == QMessageBox::Cancel) return;
        if (res == QMessageBox::Save) onSaveProject();
    }

    QString path = QFileDialog::getOpenFileName(this,
        "Select Room Model for New Project", defaultProjectDir(),
        "3D Models (*.stl *.obj);;STL files (*.stl);;OBJ files (*.obj);;All files (*.*)");
    if (path.isEmpty()) return;

    sceneManager_.clearAll();
    soundSourceFile_.clear();
    currentProjectFile_.clear();
    projectDirty_ = false;
    viewport_->clearPlacedPoints();
    assetsPanel_->clearSurfaces();

    if (viewport_->loadModel(path)) {
        updateTitle();
        statusBar()->showMessage("New project created with: " + QFileInfo(path).fileName());
    } else {
        QMessageBox::warning(this, "Error", "Failed to load 3D model: " + path);
        updateTitle();
        statusBar()->showMessage("New project created (no model)");
    }
}

void MainWindow::onOpenProject() {
    QString path = QFileDialog::getOpenFileName(this,
        "Open Project", defaultProjectDir(),
        "Room Projects (*.room);;3D Models (*.stl *.obj);;All files (*.*)");
    if (path.isEmpty()) return;

    if (path.endsWith(".stl", Qt::CaseInsensitive) || path.endsWith(".obj", Qt::CaseInsensitive)) {
        if (viewport_->loadModel(path))
            statusBar()->showMessage("Loaded: " + path);
        else
            QMessageBox::warning(this, "Error", "Failed to load 3D model: " + path);
        return;
    }

    auto data = ProjectFile::load(path);
    if (!data) {
        QMessageBox::warning(this, "Error", "Failed to load project: " + path);
        return;
    }

    if (!data->stlFilePath.isEmpty()) {
        QString stlPath = data->stlFilePath;
        if (QFileInfo(stlPath).isRelative())
            stlPath = QFileInfo(path).dir().filePath(stlPath);

        if (!viewport_->loadModel(stlPath)) {
            QMessageBox::warning(this, "Error", "Failed to load 3D model: " + stlPath);
            return;
        }
    }

    viewport_->setScaleFactor(data->scaleFactor);

    for (int i = 0; i < static_cast<int>(data->surfaceColors.size()); ++i)
        viewport_->setSurfaceColor(i, data->surfaceColors[i]);

    viewport_->clearPlacedPoints();
    viewport_->restorePlacedPoints(data->placedPoints);

    soundSourceFile_ = data->soundSourceFile;
    currentProjectFile_ = path;
    projectDirty_ = false;
    addRecentProject(path);
    updateTitle();
    statusBar()->showMessage("Project opened: " + path);
}

void MainWindow::onSaveProject() {
    if (currentProjectFile_.isEmpty()) {
        onSaveProjectAs();
        return;
    }
    saveProjectToFile(currentProjectFile_);
}

void MainWindow::onSaveProjectAs() {
    QString path = QFileDialog::getSaveFileName(this,
        "Save Project As", defaultProjectDir(), "Room Projects (*.room)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".room", Qt::CaseInsensitive))
        path += ".room";
    saveProjectToFile(path);
}

void MainWindow::saveProjectToFile(const QString& filepath) {
    ProjectData data;
    data.stlFilePath = viewport_->hasModel() ? viewport_->meshData().filePath() : QString();
    data.scaleFactor = viewport_->scaleFactor();
    data.surfaceColors = viewport_->surfaceColors();
    data.placedPoints = viewport_->placedPoints();
    data.soundSourceFile = soundSourceFile_;

    if (ProjectFile::save(filepath, data)) {
        currentProjectFile_ = filepath;
        projectDirty_ = false;
        addRecentProject(filepath);
        updateTitle();
        statusBar()->showMessage("Saved: " + filepath);
    } else {
        QMessageBox::warning(this, "Error", "Failed to save project.");
    }
}

void MainWindow::addRecentProject(const QString& filepath) {
    QSettings settings("PyRoomStudio", "PyRoomStudio");
    QStringList recent = settings.value("recentProjects").toStringList();
    recent.removeAll(filepath);
    recent.prepend(filepath);
    while (recent.size() > 5)
        recent.removeLast();
    settings.setValue("recentProjects", recent);
    updateRecentProjectsMenu();
}

void MainWindow::updateRecentProjectsMenu() {
    recentProjectsMenu_->clear();
    QSettings settings("PyRoomStudio", "PyRoomStudio");
    QStringList recent = settings.value("recentProjects").toStringList();

    if (recent.isEmpty()) {
        recentProjectsMenu_->addAction("(empty)")->setEnabled(false);
        return;
    }

    for (auto& path : recent) {
        QFileInfo fi(path);
        recentProjectsMenu_->addAction(fi.fileName(), [this, path]() {
            auto data = ProjectFile::load(path);
            if (!data) {
                QMessageBox::warning(this, "Error", "Failed to load project: " + path);
                return;
            }
            if (!data->stlFilePath.isEmpty()) {
                QString stlPath = data->stlFilePath;
                if (QFileInfo(stlPath).isRelative())
                    stlPath = QFileInfo(path).dir().filePath(stlPath);
                if (!viewport_->loadModel(stlPath)) {
                    QMessageBox::warning(this, "Error", "Failed to load 3D model: " + stlPath);
                    return;
                }
            }
            viewport_->setScaleFactor(data->scaleFactor);
            for (int i = 0; i < static_cast<int>(data->surfaceColors.size()); ++i)
                viewport_->setSurfaceColor(i, data->surfaceColors[i]);
            viewport_->clearPlacedPoints();
            viewport_->restorePlacedPoints(data->placedPoints);
            soundSourceFile_ = data->soundSourceFile;
            currentProjectFile_ = path;
            projectDirty_ = false;
            updateTitle();
            statusBar()->showMessage("Project opened: " + path);
        });
    }
    recentProjectsMenu_->addSeparator();
    recentProjectsMenu_->addAction("Clear Recent", [this]() {
        QSettings s("PyRoomStudio", "PyRoomStudio");
        s.remove("recentProjects");
        updateRecentProjectsMenu();
    });
}

void MainWindow::onExit() {
    close();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (projectDirty_) {
        auto res = QMessageBox::question(this, "Unsaved Changes",
            "Save project before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (res == QMessageBox::Cancel) { event->ignore(); return; }
        if (res == QMessageBox::Save) onSaveProject();
    }
    event->accept();
}

void MainWindow::onImportRoom() {
    QString path = QFileDialog::getOpenFileName(this,
        "Import Room Model", defaultProjectDir(),
        "3D Models (*.stl *.obj);;STL files (*.stl);;OBJ files (*.obj);;All files (*.*)");
    if (path.isEmpty()) return;

    if (viewport_->loadModel(path)) {
        statusBar()->showMessage("Loaded: " + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to load 3D model: " + path);
    }
}

void MainWindow::onImportSound() {
    QString path = QFileDialog::getOpenFileName(this,
        "Select Sound Source",
        defaultProjectDir(),
        "Audio files (*.wav *.mp3 *.flac *.ogg);;WAV files (*.wav);;All files (*.*)");
    if (path.isEmpty()) return;
    soundSourceFile_ = path;
    updateTitle();
    statusBar()->showMessage("Sound loaded: " + path);
}

void MainWindow::onPlacePoint() {
    if (!viewport_->hasModel()) {
        statusBar()->showMessage("Load a 3D model first");
        return;
    }
    viewport_->setPlacementMode(!viewport_->placementMode());
}

void MainWindow::onRender() {
    if (!viewport_->hasModel()) {
        QMessageBox::warning(this, "Error", "No 3D model loaded.");
        return;
    }

    auto [srcCount, lstCount] = viewport_->countSourcesAndListeners();
    if (srcCount == 0) {
        QMessageBox::warning(this, "Error", "No sound sources placed.\nPlace a point and set its type to Source.");
        return;
    }
    if (lstCount == 0) {
        QMessageBox::warning(this, "Error", "No listeners placed.\nPlace a point and set its type to Listener.");
        return;
    }

    std::string fallbackAudio = soundSourceFile_.toStdString();
    auto [sources, listeners] = viewport_->getSourcesAndListeners(fallbackAudio);

    QStringList missingAudio;
    for (auto& s : sources) {
        if (s.audioFile.empty())
            missingAudio << QString::fromStdString(s.name);
    }
    if (!missingAudio.isEmpty()) {
        QMessageBox::warning(this, "Error",
            QString("The following source(s) have no audio file assigned:\n  %1\n\n"
                    "Assign audio files to each source, or use Import Sound to set a default.")
            .arg(missingAudio.join(", ")));
        return;
    }

    // Show render options dialog for listener selection
    std::vector<RenderOptionsDialog::ListenerEntry> listenerEntries;
    for (auto& l : listeners)
        listenerEntries.push_back({l.name, true});

    RenderOptionsDialog renderDlg(listenerEntries, this);
    if (renderDlg.exec() != QDialog::Accepted)
        return;

    std::vector<int> selectedListeners = renderDlg.selectedListenerIndices();
    if (selectedListeners.empty()) {
        QMessageBox::warning(this, "Error", "No listeners selected for rendering.");
        return;
    }

    SceneManager simScene;
    for (auto& s : sources)
        simScene.addSoundSource(s.position, s.audioFile, s.volume, s.name);
    for (auto& l : listeners)
        simScene.addListener(l.position, l.name, l.orientation);

    QSettings settings("PyRoomStudio", "PyRoomStudio");

    SimulationWorker::Params params;
    params.scene = simScene;
    params.walls = viewport_->getWallsForAcoustic();
    params.roomCenter = viewport_->getScaledRoomCenter();
    params.modelVertices = viewport_->getScaledModelVertices();
    params.sampleRate = settings.value("audio/sampleRate", DEFAULT_SAMPLE_RATE).toInt();
    params.maxOrder = settings.value("sim/maxOrder", DEFAULT_MAX_ORDER).toInt();
    params.nRays = settings.value("sim/numRays", DEFAULT_N_RAYS).toInt();
    params.energyAbsorption = settings.value("sim/absorption", DEFAULT_ENERGY_ABSORPTION).toFloat();
    params.scattering = settings.value("sim/scattering", DEFAULT_SCATTERING).toFloat();
    params.airAbsorption = settings.value("sim/airAbsorption", true).toBool();
    params.selectedListenerIndices = selectedListeners;

    QString desc = QString("Sim: %1 src, %2 lst")
        .arg(sources.size()).arg(selectedListeners.size());
    simQueue_->enqueue(params, desc);
    statusBar()->showMessage("Simulation queued: " + desc);
}

void MainWindow::onModelLoaded(const QString& filepath) {
    populateAssetsFromRenderer(filepath);
    propertyPanel_->setScaleValue(viewport_->scaleFactor());
    Vec3f dims = viewport_->getRealWorldDimensions();
    propertyPanel_->setDimensionText(
        QString("%1 x %2 x %3 m").arg(dims.x(), 0, 'f', 1)
                                   .arg(dims.y(), 0, 'f', 1)
                                   .arg(dims.z(), 0, 'f', 1));
    updateTitle();
}

void MainWindow::onPointSelected(int index) {
    propertyPanel_->setPointControlsEnabled(true);
    if (index >= 0 && index < static_cast<int>(viewport_->placedPoints().size())) {
        auto& pt = viewport_->placedPoints()[index];
        propertyPanel_->setPointDistanceValue(pt.distance);
        propertyPanel_->setPointType(QString::fromStdString(pt.pointType));
        propertyPanel_->setPointName(QString::fromStdString(pt.name));
        propertyPanel_->setPointVolume(pt.volume);
        QFileInfo fi(QString::fromStdString(pt.audioFile));
        propertyPanel_->setPointAudioFile(fi.fileName());

        bool isListener = (pt.pointType == POINT_TYPE_LISTENER);
        propertyPanel_->setOrientationControlsVisible(isListener);
        if (isListener)
            propertyPanel_->setPointOrientationYaw(pt.orientationYaw);
    }
}

void MainWindow::onPointDeselected() {
    propertyPanel_->setPointControlsEnabled(false);
}

void MainWindow::onSurfaceSelected(int index) {
    propertyPanel_->setSurfaceControlsEnabled(true);
    auto mat = viewport_->getSurfaceMaterial(index);
    propertyPanel_->setMaterialName(mat ? QString::fromStdString(mat->name) : "");
}

void MainWindow::onSurfaceDeselected() {
    propertyPanel_->setSurfaceControlsEnabled(false);
}

void MainWindow::onPlacementModeChanged(bool enabled) {
    bottomToolbar_->setPlacePointText(enabled ? "Stop Placing" : "Place Point");
}

void MainWindow::onScaleChanged(float factor) {
    propertyPanel_->setScaleValue(factor);
    Vec3f dims = viewport_->getRealWorldDimensions();
    propertyPanel_->setDimensionText(
        QString("%1 x %2 x %3 m").arg(dims.x(), 0, 'f', 1)
                                   .arg(dims.y(), 0, 'f', 1)
                                   .arg(dims.z(), 0, 'f', 1));
}

void MainWindow::updateTitle() {
    QString title = "PyRoomStudio";
    if (!currentProjectFile_.isEmpty()) {
        QFileInfo fi(currentProjectFile_);
        title += " - " + fi.fileName();
    }
    if (projectDirty_) title += " *";
    if (!soundSourceFile_.isEmpty()) {
        QFileInfo fi(soundSourceFile_);
        title += " [" + fi.fileName() + "]";
    }
    setWindowTitle(title);
}

QString MainWindow::defaultProjectDir() const {
    QSettings s("PyRoomStudio", "PyRoomStudio");
    return s.value("defaultProjectDir", "").toString();
}

void MainWindow::configureAutoSaveTimer() {
    QSettings s("PyRoomStudio", "PyRoomStudio");
    int minutes = s.value("autoSaveInterval", 0).toInt();
    if (minutes > 0) {
        autoSaveTimer_->start(minutes * 60 * 1000);
    } else {
        autoSaveTimer_->stop();
    }
}

void MainWindow::populateAssetsFromRenderer(const QString& filepath) {
    assetsPanel_->clearSurfaces();

    const auto& colors = viewport_->surfaceColors();
    QVector<AssetsPanel::SurfaceInfo> surfaces;
    for (int i = 0; i < static_cast<int>(colors.size()); ++i) {
        Color3i displayColor = {
            static_cast<int>(colors[i][0] * 255),
            static_cast<int>(colors[i][1] * 255),
            static_cast<int>(colors[i][2] * 255)
        };
        surfaces.append({QString("Surface %1").arg(i + 1), displayColor, i});
    }

    QFileInfo fi(filepath);
    assetsPanel_->addStlSurfaces(fi.baseName(), surfaces);
}

} // namespace prs
