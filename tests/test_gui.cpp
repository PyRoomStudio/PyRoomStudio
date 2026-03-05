#include <QtTest/QtTest>
#include <QApplication>
#include <QMenuBar>
#include <QSettings>
#include <QTimer>

#include "gui/MainWindow.h"
#include "gui/PropertyPanel.h"
#include "gui/LibraryPanel.h"
#include "gui/AssetsPanel.h"
#include "gui/BottomToolbar.h"
#include "gui/UndoCommands.h"
#include "gui/widgets/ColorSwatch.h"
#include "gui/dialogs/SettingsDialogs.h"
#include "rendering/Viewport3D.h"

using namespace prs;

class TestGUI : public QObject {
    Q_OBJECT
private slots:
    void testMainWindowCreation() {
        MainWindow w;
        QVERIFY(w.windowTitle().contains("PyRoomStudio"));
        QVERIFY(w.viewport() != nullptr);
        QVERIFY(w.propertyPanel() != nullptr);
        QVERIFY(w.libraryPanel() != nullptr);
        QVERIFY(w.assetsPanel() != nullptr);
        QVERIFY(w.bottomToolbar() != nullptr);
    }

    void testMenusExist() {
        MainWindow w;
        auto* menuBar = w.menuBar();
        QVERIFY(menuBar != nullptr);
        auto actions = menuBar->actions();
        QStringList menuTitles;
        for (auto* a : actions) menuTitles << a->text();
        QVERIFY(menuTitles.contains("&File"));
        QVERIFY(menuTitles.contains("&Edit"));
        QVERIFY(menuTitles.contains("&Settings"));
    }

    void testPropertyPanelSignals() {
        PropertyPanel panel;
        QSignalSpy scaleSpy(&panel, &PropertyPanel::scaleChanged);
        QSignalSpy sourceSpy(&panel, &PropertyPanel::setPointSource);
        QSignalSpy nameSpy(&panel, &PropertyPanel::pointNameChanged);

        QVERIFY(scaleSpy.isValid());
        QVERIFY(sourceSpy.isValid());
        QVERIFY(nameSpy.isValid());
    }

    void testPropertyPanelSetters() {
        PropertyPanel panel;
        panel.setScaleValue(3.0f);
        panel.setDimensionText("5.0 x 3.0 x 2.5 m");
        panel.setPointDistanceValue(1.5f);
        panel.setPointType("source");
        panel.setMaterialName("Concrete");
        panel.setPointName("Test Point");
        panel.setPointVolume(0.75f);
        panel.setPointAudioFile("test.wav");
    }

    void testAssetsPanelSurfaces() {
        AssetsPanel panel;
        QVector<AssetsPanel::SurfaceInfo> surfaces;
        surfaces.append({"Surface 1", {200, 100, 50}, 0});
        surfaces.append({"Surface 2", {50, 100, 200}, 1});

        QSignalSpy clickSpy(&panel, &AssetsPanel::surfaceClicked);
        panel.addStlSurfaces("TestModel", surfaces);
        panel.clearSurfaces();
        QVERIFY(true);
    }

    void testBottomToolbarButtons() {
        BottomToolbar toolbar;
        QSignalSpy importRoomSpy(&toolbar, &BottomToolbar::importRoomClicked);
        QSignalSpy importSoundSpy(&toolbar, &BottomToolbar::importSoundClicked);
        QSignalSpy placePointSpy(&toolbar, &BottomToolbar::placePointClicked);
        QSignalSpy renderSpy(&toolbar, &BottomToolbar::renderClicked);

        QVERIFY(importRoomSpy.isValid());
        QVERIFY(importSoundSpy.isValid());
        QVERIFY(placePointSpy.isValid());
        QVERIFY(renderSpy.isValid());
    }

    void testColorSwatchSignal() {
        ColorSwatch swatch("Test", {200, 100, 50});
        QSignalSpy spy(&swatch, &ColorSwatch::clicked);
        QVERIFY(spy.isValid());
        QTest::mouseClick(&swatch, Qt::LeftButton);
        QCOMPARE(spy.count(), 1);
    }

    void testLibraryPanelSignals() {
        LibraryPanel panel;
        QSignalSpy matSpy(&panel, &LibraryPanel::materialSelected);
        QSignalSpy sndSpy(&panel, &LibraryPanel::soundFileSelected);
        QVERIFY(matSpy.isValid());
        QVERIFY(sndSpy.isValid());
    }

    void testWindowTitleUpdates() {
        MainWindow w;
        QVERIFY(w.windowTitle() == "PyRoomStudio");
    }

    // ==================== New Feature Tests ====================

    void testDisplaySettingsApplied() {
        QSettings s("PyRoomStudio", "PyRoomStudio");
        s.setValue("display/gridVisible", false);
        s.setValue("display/gridSpacing", 2.5);
        s.setValue("display/transparencyAlpha", 0.3);
        s.setValue("display/markerSize", 25);

        Viewport3D vp;
        vp.applyDisplaySettings();

        QCOMPARE(vp.gridVisible(), false);
        QVERIFY(std::abs(vp.gridSpacing() - 2.5f) < 0.01f);
        QVERIFY(std::abs(vp.transparencyAlpha() - 0.3f) < 0.01f);
        QCOMPARE(vp.markerSize(), 25);

        s.setValue("display/gridVisible", true);
        s.setValue("display/gridSpacing", 1.0);
        s.setValue("display/transparencyAlpha", 0.55);
        s.setValue("display/markerSize", 15);
    }

    void testDisplaySettingsSignalEmitted() {
        DisplaySettingsDialog dlg;
        QSignalSpy spy(&dlg, &DisplaySettingsDialog::settingsChanged);
        QVERIFY(spy.isValid());
    }

    void testDefaultProjectDirPreference() {
        QSettings s("PyRoomStudio", "PyRoomStudio");
        s.setValue("defaultProjectDir", "/tmp/test_projects");

        MainWindow w;
        s.setValue("defaultProjectDir", "");
    }

    void testAutoSaveTimerSetup() {
        QSettings s("PyRoomStudio", "PyRoomStudio");
        s.setValue("autoSaveInterval", 5);

        MainWindow w;
        auto* timer = w.findChild<QTimer*>();
        QVERIFY(timer != nullptr);
        QVERIFY(timer->isActive());
        QCOMPARE(timer->interval(), 5 * 60 * 1000);

        s.setValue("autoSaveInterval", 0);
    }

    void testAutoSaveTimerDisabled() {
        QSettings s("PyRoomStudio", "PyRoomStudio");
        s.setValue("autoSaveInterval", 0);

        MainWindow w;
        auto* timer = w.findChild<QTimer*>();
        QVERIFY(timer != nullptr);
        QVERIFY(!timer->isActive());
    }

    void testSelectAllPoints() {
        Viewport3D vp;

        PlacedPoint p1;
        p1.surfacePoint = Vec3f(1, 0, 0);
        p1.normal = Vec3f(0, 0, 1);
        PlacedPoint p2;
        p2.surfacePoint = Vec3f(2, 0, 0);
        p2.normal = Vec3f(0, 0, 1);
        PlacedPoint p3;
        p3.surfacePoint = Vec3f(3, 0, 0);
        p3.normal = Vec3f(0, 0, 1);

        vp.placedPoints().push_back(p1);
        vp.placedPoints().push_back(p2);
        vp.placedPoints().push_back(p3);

        QCOMPARE(static_cast<int>(vp.selectedPointIndices().size()), 0);

        vp.selectAllPoints();
        QCOMPARE(static_cast<int>(vp.selectedPointIndices().size()), 3);
        QVERIFY(vp.selectedPointIndices().count(0));
        QVERIFY(vp.selectedPointIndices().count(1));
        QVERIFY(vp.selectedPointIndices().count(2));

        vp.clearPointSelection();
        QCOMPARE(static_cast<int>(vp.selectedPointIndices().size()), 0);
    }

    void testSelectAllClearedOnRestore() {
        Viewport3D vp;

        PlacedPoint p1;
        p1.surfacePoint = Vec3f(1, 0, 0);
        p1.normal = Vec3f(0, 0, 1);
        vp.placedPoints().push_back(p1);
        vp.selectAllPoints();
        QCOMPARE(static_cast<int>(vp.selectedPointIndices().size()), 1);

        vp.clearPlacedPoints();
        QCOMPARE(static_cast<int>(vp.selectedPointIndices().size()), 0);
    }

    void testMoveMode() {
        Viewport3D vp;
        QCOMPARE(vp.moveMode(), false);
        vp.setMoveMode(true);
        QCOMPARE(vp.moveMode(), true);
        vp.setMoveMode(false);
        QCOMPARE(vp.moveMode(), false);
    }

    void testMovePointCommand() {
        Viewport3D vp;

        PlacedPoint pt;
        pt.surfacePoint = Vec3f(1, 0, 0);
        pt.normal = Vec3f(0, 0, 1);
        pt.distance = 0.5f;
        vp.placedPoints().push_back(pt);

        PlacedPoint oldState = pt;
        PlacedPoint newState = pt;
        newState.surfacePoint = Vec3f(3, 2, 0);
        newState.normal = Vec3f(0, 1, 0);

        QUndoStack stack;
        stack.push(new MovePointCommand(&vp, 0, oldState, newState));

        QVERIFY((vp.placedPoints()[0].surfacePoint - Vec3f(3, 2, 0)).norm() < 1e-4f);
        QVERIFY((vp.placedPoints()[0].normal - Vec3f(0, 1, 0)).norm() < 1e-4f);

        stack.undo();
        QVERIFY((vp.placedPoints()[0].surfacePoint - Vec3f(1, 0, 0)).norm() < 1e-4f);
        QVERIFY((vp.placedPoints()[0].normal - Vec3f(0, 0, 1)).norm() < 1e-4f);

        stack.redo();
        QVERIFY((vp.placedPoints()[0].surfacePoint - Vec3f(3, 2, 0)).norm() < 1e-4f);
    }

    void testMoveToolEnabled() {
        MainWindow w;
        auto toolbars = w.findChildren<QToolBar*>();
        QVERIFY(!toolbars.isEmpty());
        bool foundMove = false;
        for (auto* toolbar : toolbars) {
            for (auto* a : toolbar->actions()) {
                if (a->text() == "Move") {
                    foundMove = true;
                    QVERIFY(a->isEnabled());
                    QVERIFY(a->isCheckable());
                }
            }
        }
        QVERIFY(foundMove);
    }
};

QTEST_MAIN(TestGUI)
#include "test_gui.moc"
