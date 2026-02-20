#include "MainWindow.h"
#include "LibraryPanel.h"
#include "PropertyPanel.h"
#include "AssetsPanel.h"
#include "BottomToolbar.h"
#include "acoustics/AcousticSimulator.h"

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

namespace prs {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("PyRoomStudio");
    resize(1200, 800);

    setupMenus();
    setupToolbars();
    setupCentralWidget();
    connectSignals();

    statusBar()->showMessage("Ready");
}

void MainWindow::setupMenus() {
    auto* fileMenu = menuBar()->addMenu("&File");
    actNewProject_  = fileMenu->addAction("&New Project",  this, &MainWindow::onNewProject);
    actOpenProject_ = fileMenu->addAction("&Open Project", QKeySequence::Open, this, &MainWindow::onOpenProject);
    actSaveProject_ = fileMenu->addAction("&Save Project", QKeySequence::Save, this, &MainWindow::onSaveProject);
    actSaveProject_->setEnabled(false);
    fileMenu->addSeparator();
    fileMenu->addAction("Import...")->setEnabled(false);
    fileMenu->addAction("Export...")->setEnabled(false);
    fileMenu->addAction("Recent Projects")->setEnabled(false);
    fileMenu->addSeparator();
    actExit_ = fileMenu->addAction("E&xit", QKeySequence::Quit, this, &MainWindow::onExit);

    auto* editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Undo")->setEnabled(false);
    editMenu->addAction("Redo")->setEnabled(false);
    editMenu->addSeparator();
    editMenu->addAction("Cut")->setEnabled(false);
    editMenu->addAction("Copy")->setEnabled(false);
    editMenu->addAction("Paste")->setEnabled(false);
    editMenu->addAction("Delete")->setEnabled(false);
    editMenu->addSeparator();
    editMenu->addAction("Select All")->setEnabled(false);
    editMenu->addAction("Find")->setEnabled(false);

    auto* settingsMenu = menuBar()->addMenu("&Settings");
    settingsMenu->addAction("Preferences")->setEnabled(false);
    settingsMenu->addAction("Display Settings")->setEnabled(false);
    settingsMenu->addAction("Audio Settings")->setEnabled(false);
    settingsMenu->addAction("Keyboard Shortcuts")->setEnabled(false);
}

void MainWindow::setupToolbars() {
    topToolbar_ = addToolBar("Tools");
    topToolbar_->setMovable(false);
    auto addDisabledAction = [&](const QString& text) {
        QAction* a = topToolbar_->addAction(text);
        a->setEnabled(false);
        return a;
    };
    addDisabledAction("Move");
    addDisabledAction("Copy");
    addDisabledAction("Cut");
    addDisabledAction("Paste");
    addDisabledAction("Delete");
    addDisabledAction("Measure");
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

    // Bottom toolbar
    bottomToolbar_ = new BottomToolbar;
    mainLayout->addWidget(bottomToolbar_);

    setCentralWidget(centralWidget);
}

void MainWindow::connectSignals() {
    // Viewport signals
    connect(viewport_, &Viewport3D::modelLoaded,           this, &MainWindow::onModelLoaded);
    connect(viewport_, &Viewport3D::pointSelected,         this, &MainWindow::onPointSelected);
    connect(viewport_, &Viewport3D::pointDeselected,       this, &MainWindow::onPointDeselected);
    connect(viewport_, &Viewport3D::surfaceSelected,       this, &MainWindow::onSurfaceSelected);
    connect(viewport_, &Viewport3D::surfaceDeselected,     this, &MainWindow::onSurfaceDeselected);
    connect(viewport_, &Viewport3D::placementModeChanged,  this, &MainWindow::onPlacementModeChanged);
    connect(viewport_, &Viewport3D::scaleChanged,          this, &MainWindow::onScaleChanged);

    // Bottom toolbar
    connect(bottomToolbar_, &BottomToolbar::importRoomClicked,  this, &MainWindow::onImportRoom);
    connect(bottomToolbar_, &BottomToolbar::importSoundClicked, this, &MainWindow::onImportSound);
    connect(bottomToolbar_, &BottomToolbar::placePointClicked,  this, &MainWindow::onPlacePoint);
    connect(bottomToolbar_, &BottomToolbar::renderClicked,      this, &MainWindow::onRender);

    // Property panel -> viewport
    connect(propertyPanel_, &PropertyPanel::scaleChanged, [this](float v) {
        viewport_->setScaleFactor(v);
    });
    connect(propertyPanel_, &PropertyPanel::pointDistanceChanged, [this](float v) {
        viewport_->updateActivePointDistance(v);
    });
    connect(propertyPanel_, &PropertyPanel::setPointSource, [this]() {
        viewport_->setActivePointType(POINT_TYPE_SOURCE);
        viewport_->update();
    });
    connect(propertyPanel_, &PropertyPanel::setPointListener, [this]() {
        viewport_->setActivePointType(POINT_TYPE_LISTENER);
        viewport_->update();
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
    connect(propertyPanel_, &PropertyPanel::deselectSurface, [this]() {
        viewport_->deselectSurface();
    });

    // Library panel -> viewport (material selection)
    connect(libraryPanel_, &LibraryPanel::materialSelected,
        [this](const QString&, const Color3f& color, float) {
            int si = viewport_->selectedSurfaceIndex();
            if (si >= 0) viewport_->setSurfaceColor(si, color);
        });
}

// ==================== Slots ====================

void MainWindow::onNewProject() {
    sceneManager_.clearAll();
    soundSourceFile_.clear();
    viewport_->clearPlacedPoints();
    assetsPanel_->clearSurfaces();
    setWindowTitle("PyRoomStudio");
    statusBar()->showMessage("New project created");
}

void MainWindow::onOpenProject() {
    onImportRoom();
}

void MainWindow::onSaveProject() {
    statusBar()->showMessage("Save Project: Future feature");
}

void MainWindow::onExit() {
    close();
}

void MainWindow::onImportRoom() {
    QString path = QFileDialog::getOpenFileName(this,
        "Open STL File", QString(), "STL files (*.stl);;All files (*.*)");
    if (path.isEmpty()) return;

    if (viewport_->loadModel(path)) {
        statusBar()->showMessage("Loaded: " + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to load STL file: " + path);
    }
}

void MainWindow::onImportSound() {
    QString path = QFileDialog::getOpenFileName(this,
        "Select Sound Source",
        QString(),
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
    if (soundSourceFile_.isEmpty()) {
        QMessageBox::warning(this, "Error", "No sound file loaded.\nUse Import Sound first.");
        return;
    }

    setWindowTitle("PyRoomStudio - Simulating...");
    QApplication::processEvents();

    auto [sources, listeners] = viewport_->getSourcesAndListeners(soundSourceFile_.toStdString());

    sceneManager_.clearAll();
    for (auto& s : sources)
        sceneManager_.addSoundSource(s.position, s.audioFile, s.volume, s.name);
    for (auto& l : listeners)
        sceneManager_.addListener(l.position, l.name);

    auto walls          = viewport_->getWallsForAcoustic();
    auto roomCenter     = viewport_->getScaledRoomCenter();
    auto modelVertices  = viewport_->getScaledModelVertices();

    AcousticSimulator simulator;
    QString outputDir = simulator.simulateScene(
        sceneManager_, walls, roomCenter, modelVertices,
        DEFAULT_MAX_ORDER, DEFAULT_N_RAYS, DEFAULT_ENERGY_ABSORPTION, DEFAULT_SCATTERING);

    updateTitle();

    if (!outputDir.isEmpty()) {
        QMessageBox::information(this, "Simulation Complete",
            "Output saved to:\n" + outputDir);
        statusBar()->showMessage("Simulation complete: " + outputDir);
    } else {
        QMessageBox::warning(this, "Simulation Failed", "The acoustic simulation failed.");
    }
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
    }
}

void MainWindow::onPointDeselected() {
    propertyPanel_->setPointControlsEnabled(false);
}

void MainWindow::onSurfaceSelected(int index) {
    Q_UNUSED(index);
    propertyPanel_->setSurfaceControlsEnabled(true);
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
    if (!soundSourceFile_.isEmpty()) {
        QFileInfo fi(soundSourceFile_);
        title += " - Sound: " + fi.fileName();
    }
    setWindowTitle(title);
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
