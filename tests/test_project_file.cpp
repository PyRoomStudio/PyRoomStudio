#include <QtTest/QtTest>
#include <QTemporaryFile>
#include "core/ProjectFile.h"

using namespace prs;

class TestProjectFile : public QObject {
    Q_OBJECT
private slots:
    void testSaveAndLoadRoundTrip() {
        ProjectData data;
        data.stlFilePath = "model.stl";
        data.scaleFactor = 2.5f;
        data.soundSourceFile = "test.wav";
        data.surfaceColors = {{0.1f, 0.2f, 0.3f}, {0.4f, 0.5f, 0.6f}};

        Material mat;
        mat.name = "Concrete";
        mat.absorption = {0.02f, 0.03f, 0.03f, 0.03f, 0.04f, 0.07f};
        mat.scattering = 0.15f;
        data.surfaceMaterials = {mat, std::nullopt};

        PlacedPoint pt;
        pt.surfacePoint = Vec3f(1, 2, 3);
        pt.normal = Vec3f(0, 0, 1);
        pt.distance = 0.5f;
        pt.pointType = POINT_TYPE_SOURCE;
        pt.name = "MySource";
        pt.volume = 0.8f;
        pt.audioFile = "sound.wav";
        data.placedPoints.push_back(pt);

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        tmp.setFileTemplate(QDir::tempPath() + "/testXXXXXX.room");
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(ProjectFile::save(path, data));

        auto loaded = ProjectFile::load(path);
        QVERIFY(loaded.has_value());

        QCOMPARE(loaded->stlFilePath, QString("model.stl"));
        QVERIFY(std::abs(loaded->scaleFactor - 2.5f) < 1e-5f);
        QCOMPARE(loaded->soundSourceFile, QString("test.wav"));
        QCOMPARE(static_cast<int>(loaded->surfaceColors.size()), 2);
        QVERIFY(std::abs(loaded->surfaceColors[0][0] - 0.1f) < 1e-5f);

        QCOMPARE(static_cast<int>(loaded->surfaceMaterials.size()), 2);
        QVERIFY(loaded->surfaceMaterials[0].has_value());
        QCOMPARE(loaded->surfaceMaterials[0]->name, std::string("Concrete"));
        QVERIFY(!loaded->surfaceMaterials[1].has_value());

        QCOMPARE(static_cast<int>(loaded->placedPoints.size()), 1);
        auto& lpt = loaded->placedPoints[0];
        QVERIFY(std::abs(lpt.surfacePoint.x() - 1.0f) < 1e-5f);
        QCOMPARE(lpt.pointType, std::string(POINT_TYPE_SOURCE));
        QCOMPARE(lpt.name, std::string("MySource"));
        QVERIFY(std::abs(lpt.volume - 0.8f) < 1e-5f);
        QCOMPARE(lpt.audioFile, std::string("sound.wav"));
    }

    void testLoadMissingFile() {
        auto result = ProjectFile::load("/nonexistent/path/file.room");
        QVERIFY(!result.has_value());
    }

    void testLoadCorruptJson() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        tmp.write("{ not valid json !!!");
        QString path = tmp.fileName();
        tmp.close();

        auto result = ProjectFile::load(path);
        QVERIFY(!result.has_value());
    }

    void testEmptyProject() {
        ProjectData data;
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        tmp.setFileTemplate(QDir::tempPath() + "/testXXXXXX.room");
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(ProjectFile::save(path, data));
        auto loaded = ProjectFile::load(path);
        QVERIFY(loaded.has_value());
        QVERIFY(loaded->placedPoints.empty());
        QVERIFY(loaded->surfaceColors.empty());
    }
};

QTEST_MAIN(TestProjectFile)
#include "test_project_file.moc"
