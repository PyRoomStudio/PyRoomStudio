#include <QtTest/QtTest>
#include <QApplication>
#include <QMenuBar>

#include "gui/MainWindow.h"
#include "gui/PropertyPanel.h"
#include "gui/LibraryPanel.h"
#include "gui/AssetsPanel.h"
#include "gui/BottomToolbar.h"
#include "gui/widgets/ColorSwatch.h"

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
};

QTEST_MAIN(TestGUI)
#include "test_gui.moc"
