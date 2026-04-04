#include "core/ProjectFile.h"

#include <QFile>
#include <QTemporaryFile>
#include <QtTest/QtTest>

using namespace prs;

class TestProjectFile : public QObject {
    Q_OBJECT
  private slots:
    void testSaveAndLoadRoundTrip() {
        ProjectData data;
        data.stlFilePath = "model.stl";
        data.scaleFactor = 2.5f;
        data.sampleRate = 48000;
        data.soundSourceFile = "test.wav";
        data.surfaceColors = {{0.1f, 0.2f, 0.3f}, {0.4f, 0.5f, 0.6f}};

        Material mat;
        mat.name = "Concrete";
        mat.thickness = "100mm";
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
        QCOMPARE(loaded->sampleRate, 48000);
        QCOMPARE(loaded->soundSourceFile, QString("test.wav"));
        QCOMPARE(static_cast<int>(loaded->surfaceColors.size()), 2);
        QVERIFY(std::abs(loaded->surfaceColors[0][0] - 0.1f) < 1e-5f);

        QCOMPARE(static_cast<int>(loaded->surfaceMaterials.size()), 2);
        QVERIFY(loaded->surfaceMaterials[0].has_value());
        QCOMPARE(loaded->surfaceMaterials[0]->name, std::string("Concrete"));
        QCOMPARE(loaded->surfaceMaterials[0]->thickness, std::string("100mm"));
        QVERIFY(!loaded->surfaceMaterials[1].has_value());

        QCOMPARE(static_cast<int>(loaded->placedPoints.size()), 1);
        auto& lpt = loaded->placedPoints[0];
        QVERIFY(std::abs(lpt.surfacePoint.x() - 1.0f) < 1e-5f);
        QCOMPARE(lpt.pointType, std::string(POINT_TYPE_SOURCE));
        QCOMPARE(lpt.name, std::string("MySource"));
        QVERIFY(std::abs(lpt.volume - 0.8f) < 1e-5f);
        QCOMPARE(lpt.audioFile, std::string("sound.wav"));
    }

    void testLoadRejectsMissingRequiredFields() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        tmp.write(
            R"({"version":1,"stlFilePath":"model.stl","scaleFactor":1,"soundSourceFile":"","surfaceColors":[],"surfaceMaterials":[]})");
        QString path = tmp.fileName();
        tmp.close();

        auto result = ProjectFile::load(path);
        QVERIFY(!result.has_value());
    }

    void testLoadRejectsMismatchedSurfaceMaterials() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        tmp.write(
            R"({"version":1,"stlFilePath":"model.stl","scaleFactor":1,"soundSourceFile":"","surfaceColors":[[0.1,0.2,0.3],[0.4,0.5,0.6]],"surfaceMaterials":[null],"placedPoints":[]})");
        QString path = tmp.fileName();
        tmp.close();

        auto result = ProjectFile::load(path);
        QVERIFY(!result.has_value());
    }

    void testSaveIsDeterministic() {
        ProjectData data;
        data.stlFilePath = "model.stl";
        data.scaleFactor = 1.0f;
        data.sampleRate = 44100;
        data.soundSourceFile = "audio.wav";
        data.surfaceColors = {{0.25f, 0.5f, 0.75f}};

        QTemporaryFile fileA;
        QTemporaryFile fileB;
        fileA.setAutoRemove(true);
        fileB.setAutoRemove(true);
        QVERIFY(fileA.open());
        QVERIFY(fileB.open());
        QString pathA = fileA.fileName();
        QString pathB = fileB.fileName();
        fileA.close();
        fileB.close();

        QVERIFY(ProjectFile::save(pathA, data));
        QVERIFY(ProjectFile::save(pathB, data));

        QFile savedA(pathA);
        QFile savedB(pathB);
        QVERIFY(savedA.open(QIODevice::ReadOnly));
        QVERIFY(savedB.open(QIODevice::ReadOnly));
        QCOMPARE(savedA.readAll(), savedB.readAll());
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

    void testSaveRejectsInvalidSampleRate() {
        ProjectData data;
        data.sampleRate = -1;
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(!ProjectFile::save(path, data));
    }

    void testLoadRejectsInvalidPlacedPointColor() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        tmp.write(
            R"({"version":1,"stlFilePath":"model.stl","scaleFactor":1,"soundSourceFile":"","sampleRate":44100,"surfaceColors":[[0.1,0.2,0.3]],"surfaceMaterials":[null],"placedPoints":[{"surfacePoint":{"x":0,"y":0,"z":0},"normal":{"x":0,"y":0,"z":1},"distance":0,"color":[-1,0,0],"pointType":"source","name":"","volume":1,"audioFile":"","orientationYaw":0}]})");
        QString path = tmp.fileName();
        tmp.close();

        auto result = ProjectFile::load(path);
        QVERIFY(!result.has_value());
    }

    void testLoadDefaultsSampleRate() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        tmp.write(
            R"({"version":1,"stlFilePath":"default.stl","scaleFactor":1,"soundSourceFile":"","surfaceColors":[[0.1,0.2,0.3]],"surfaceMaterials":[null],"placedPoints":[]})");
        QString path = tmp.fileName();
        tmp.close();

        auto result = ProjectFile::load(path);
        QVERIFY(result.has_value());
        QCOMPARE(result->sampleRate, DEFAULT_SAMPLE_RATE);
    }
};

QTEST_MAIN(TestProjectFile)
#include "test_project_file.moc"
