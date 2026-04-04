#include "audio/AudioFile.h"
#include "audio/SignalProcessing.h"
#include "core/MaterialLoader.h"
#include "core/ProjectFile.h"
#include "utils/ResourcePath.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest/QtTest>

#include <cmath>
#include <limits>

using namespace prs;

namespace {

struct CurrentDirGuard {
    QString original;
    ~CurrentDirGuard() {
        QDir::setCurrent(original);
    }
};

bool writeTextFile(const QString& path, const QByteArray& contents) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    if (file.write(contents) != contents.size())
        return false;
    return true;
}

QString makeLegacyProjectJson(int sampleRate, bool includeSampleRate) {
    QJsonObject root;
    root["version"] = 1;
    root["stlFilePath"] = "model.stl";
    root["scaleFactor"] = 1.0;
    if (includeSampleRate)
        root["sampleRate"] = sampleRate;
    root["soundSourceFile"] = "source.wav";
    root["surfaceColors"] = QJsonArray{QJsonArray{0.1, 0.2, 0.3}};
    root["surfaceMaterials"] = QJsonArray{QJsonValue()};
    root["placedPoints"] = QJsonArray();
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

} // namespace

class TestCoverage : public QObject {
    Q_OBJECT
  private slots:
    void testMaterialLoaderDirectoryAndFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QDir root(tempDir.path());
        QVERIFY(root.mkpath("acoustic_treatments"));

        Material wall;
        wall.name = "Brick";
        wall.thickness = "50mm";
        wall.texturePath = "textures/brick.png";
        wall.scattering = 0.25f;
        wall.absorption = {0.15f, 0.16f, 0.17f, 0.18f, 0.19f, 0.2f};
        QVERIFY(MaterialLoader::saveToFile(root.filePath("acoustic_treatments/wall.mat"), wall));

        Material curtain;
        curtain.name = "Curtain";
        curtain.thickness = "10mm";
        curtain.texturePath = "textures/curtain.png";
        curtain.scattering = 0.05f;
        curtain.absorption = {0.55f, 0.56f, 0.57f, 0.58f, 0.59f, 0.6f};
        QVERIFY(MaterialLoader::saveToFile(root.filePath("ceiling.mat"), curtain));

        QVERIFY(writeTextFile(root.filePath("acoustic_treatments/bad.mat"), "{ not valid json"));

        auto categories = MaterialLoader::loadFromDirectory(tempDir.path());
        QCOMPARE(static_cast<int>(categories.size()), 2);

        QCOMPARE(categories[0].name, std::string("Acoustic Treatments"));
        QCOMPARE(static_cast<int>(categories[0].materials.size()), 1);
        QCOMPARE(categories[0].materials[0].name, std::string("Brick"));
        QCOMPARE(categories[0].materials[0].category, std::string("Acoustic Treatments"));
        QCOMPARE(categories[0].materials[0].texturePath, std::string("textures/brick.png"));

        QCOMPARE(categories[1].name, std::string("Other"));
        QCOMPARE(static_cast<int>(categories[1].materials.size()), 1);
        QCOMPARE(categories[1].materials[0].name, std::string("Curtain"));
        QCOMPARE(categories[1].materials[0].category, std::string("Other"));
        QVERIFY(categories[1].materials[0].averageAbsorption() > 0.0f);

        QVERIFY(MaterialLoader::loadFromFile(root.filePath("acoustic_treatments/wall.mat")).has_value());
        QVERIFY(!MaterialLoader::loadFromFile(root.filePath("missing.mat")).has_value());
    }

    void testResourcePathFallbacks() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString originalDir = QDir::currentPath();
        CurrentDirGuard guard{originalDir};
        QVERIFY(QDir::setCurrent(tempDir.path()));

        QFile asset(tempDir.filePath("texture.png"));
        QVERIFY(asset.open(QIODevice::WriteOnly));
        QVERIFY(asset.write("png") == 3);
        asset.close();

        const QString resolved = resourcePath("texture.png");
        QCOMPARE(QFileInfo(resolved).absoluteFilePath(), QFileInfo(asset).absoluteFilePath());
        QCOMPARE(resolveMaterialTexturePath("texture.png"), resolved);
        QCOMPARE(resolveMaterialTexturePath(""), QString());
        QCOMPARE(resolveMaterialTexturePath("does-not-exist.png"), QString("does-not-exist.png"));
    }

    void testProjectFileValidationAndLegacyDefaults() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        const QString path = tmp.fileName();
        tmp.close();

        ProjectData invalidRate;
        invalidRate.stlFilePath = "model.stl";
        invalidRate.scaleFactor = 1.0f;
        invalidRate.sampleRate = 0;
        invalidRate.soundSourceFile = "source.wav";
        invalidRate.surfaceColors = {{0.1f, 0.2f, 0.3f}};
        invalidRate.surfaceMaterials = {std::nullopt};
        QVERIFY(!ProjectFile::save(path, invalidRate));

        ProjectData valid;
        valid.stlFilePath = "model.stl";
        valid.scaleFactor = 1.0f;
        valid.sampleRate = 22050;
        valid.soundSourceFile = "source.wav";
        valid.surfaceColors = {{0.1f, 0.2f, 0.3f}};
        valid.surfaceMaterials = {std::nullopt};

        QVERIFY(ProjectFile::save(path, valid));

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object().value("sampleRate").toInt(), 22050);

        QTemporaryFile legacy;
        legacy.setAutoRemove(true);
        QVERIFY(legacy.open());
        QVERIFY(writeTextFile(legacy.fileName(), makeLegacyProjectJson(DEFAULT_SAMPLE_RATE, false).toUtf8()));
        const QString legacyPath = legacy.fileName();
        legacy.close();

        auto legacyLoaded = ProjectFile::load(legacyPath);
        QVERIFY(legacyLoaded.has_value());
        QCOMPARE(legacyLoaded->sampleRate, DEFAULT_SAMPLE_RATE);

        QTemporaryFile badPoint;
        badPoint.setAutoRemove(true);
        QVERIFY(badPoint.open());
        QVERIFY(writeTextFile(
            badPoint.fileName(),
            R"({"version":1,"stlFilePath":"model.stl","scaleFactor":1,"soundSourceFile":"source.wav","surfaceColors":[[0.1,0.2,0.3]],"surfaceMaterials":[null],"placedPoints":[{"surfacePoint":{"x":0,"y":0,"z":0},"normal":{"x":0,"y":0,"z":1},"distance":0,"color":[0.1,0.2,0.3],"pointType":"speaker","name":"bad","volume":1,"audioFile":"","orientationYaw":0}]})"));
        const QString badPointPath = badPoint.fileName();
        badPoint.close();

        auto badLoaded = ProjectFile::load(badPointPath);
        QVERIFY(!badLoaded.has_value());
    }

    void testSignalProcessingEdgeCases() {
        std::vector<float> zeroSignal = {0.0f, 0.0f, 0.0f};
        SignalProcessing::normalize(zeroSignal, 0.5f);
        QCOMPARE(zeroSignal[0], 0.0f);
        QCOMPARE(zeroSignal[1], 0.0f);

        QVERIFY(SignalProcessing::convolve({}, {1.0f}).empty());
        QVERIFY(SignalProcessing::convolve({1.0f}, {}).empty());
        QVERIFY(SignalProcessing::bandpassFilter({}, 44100, 1000.0f).empty());

        std::vector<float> passthrough = {1.0f, 2.0f, 3.0f};
        auto degenerate = SignalProcessing::bandpassFilter(passthrough, 44100, 40000.0f);
        QCOMPARE(degenerate.size(), passthrough.size());
        QCOMPARE(degenerate[0], passthrough[0]);
        QCOMPARE(degenerate[1], passthrough[1]);

        std::array<std::vector<float>, NUM_FREQ_BANDS> emptyBands{};
        QVERIFY(SignalProcessing::combineMultibandRIR(emptyBands, 44100).empty());

        std::array<std::vector<float>, NUM_FREQ_BANDS> bands{};
        bands[0] = {1.0f, 0.5f, 0.0f};
        auto combined = SignalProcessing::combineMultibandRIR(bands, 44100);
        QVERIFY(!combined.empty());
        QVERIFY(combined.size() >= bands[0].size());
    }

    void testAudioFileRejectsInvalidStereoInput() {
        QVERIFY(!AudioFile::saveStereo("/nonexistent/output.wav", 44100, {1.0f}, {0.5f, 0.25f}));
        QVERIFY(!AudioFile::saveStereo("/nonexistent/output.wav", 44100, {}, {}));
    }
};

QTEST_MAIN(TestCoverage)
#include "test_coverage.moc"
