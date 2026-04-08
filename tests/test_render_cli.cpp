#include "audio/AudioFile.h"
#include "core/PlacedPoint.h"
#include "core/ProjectFile.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace prs;

namespace {

bool writeTextFile(const QString& path, const QByteArray& contents) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(contents) == contents.size();
}

QString cliExecutablePath() {
#ifdef Q_OS_WIN
    const QString name = "SeicheCLI.exe";
#else
    const QString name = "SeicheCLI";
#endif
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString local = appDir.filePath(name);
    if (QFileInfo::exists(local))
        return local;
    return appDir.filePath(QStringLiteral("../%1").arg(name));
}

void writeSquarePlaneObj(const QString& path) {
    const QByteArray obj = R"(v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
f 1 2 3
f 1 3 4
)";
    QVERIFY(writeTextFile(path, obj));
}

void writeFixtureAudio(const QString& path) {
    AudioFile af;
    af.samples() = {0.0f, 0.25f, 0.5f, 0.25f, 0.0f, -0.25f, -0.5f, -0.25f};
    QVERIFY(af.save(path, 22050));
}

ProjectData makeProject() {
    ProjectData data;
    data.stlFilePath = "model.obj";
    data.scaleFactor = 1.0f;
    data.sampleRate = 22050;
    data.soundSourceFile = "source.wav";
    data.surfaceColors = {{0.2f, 0.4f, 0.6f}};
    data.surfaceMaterials = {std::nullopt};

    PlacedPoint source;
    source.surfacePoint = Vec3f(0.25f, 0.25f, 0.0f);
    source.normal = Vec3f(0.0f, 0.0f, 1.0f);
    source.distance = 0.1f;
    source.pointType = POINT_TYPE_SOURCE;
    source.name = "Source";
    source.volume = 1.0f;
    source.audioFile.clear();
    data.placedPoints.push_back(source);

    PlacedPoint listener;
    listener.surfacePoint = Vec3f(0.75f, 0.75f, 0.0f);
    listener.normal = Vec3f(0.0f, 0.0f, 1.0f);
    listener.distance = 0.1f;
    listener.pointType = POINT_TYPE_LISTENER;
    listener.name = "Listener";
    listener.orientationYaw = 0.0f;
    data.placedPoints.push_back(listener);

    return data;
}

} // namespace

class TestRenderCli : public QObject {
    Q_OBJECT
  private slots:

    void initTestCase() {
        cliPath_ = cliExecutablePath();
        QVERIFY2(QFileInfo::exists(cliPath_), qPrintable(QString("Missing CLI executable: %1").arg(cliPath_)));
    }

    // -- Error-handling tests ------------------------------------------------

    void testNoArguments() {
        QProcess proc;
        proc.setProgram(cliPath_);
        proc.start();
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitCode(), 1);
        QVERIFY(proc.readAllStandardError().contains("--project"));
    }

    void testMissingProjectFile() {
        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--project", "/nonexistent/path/does_not_exist.room"});
        proc.start();
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitCode(), 1);
        QVERIFY(proc.readAllStandardError().contains("Failed to load"));
    }

    void testInvalidMethod() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString projectDir = tempDir.path();
        writeSquarePlaneObj(QDir(projectDir).filePath("model.obj"));
        writeFixtureAudio(QDir(projectDir).filePath("source.wav"));
        QVERIFY(ProjectFile::save(QDir(projectDir).filePath("test.room"), makeProject()));

        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--project", QDir(projectDir).filePath("test.room"), "--method", "bogus"});
        proc.start();
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitCode(), 1);
        QVERIFY(proc.readAllStandardError().contains("--method"));
    }

    void testInvalidAirAbsorption() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString projectDir = tempDir.path();
        writeSquarePlaneObj(QDir(projectDir).filePath("model.obj"));
        writeFixtureAudio(QDir(projectDir).filePath("source.wav"));
        QVERIFY(ProjectFile::save(QDir(projectDir).filePath("test.room"), makeProject()));

        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--project", QDir(projectDir).filePath("test.room"), "--air-absorption", "maybe"});
        proc.start();
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitCode(), 1);
        QVERIFY(proc.readAllStandardError().contains("--air-absorption"));
    }

    void testCorruptProjectFile() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QVERIFY(writeTextFile(QDir(tempDir.path()).filePath("bad.room"), "{ not valid json !!!"));

        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--project", QDir(tempDir.path()).filePath("bad.room")});
        proc.start();
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitCode(), 1);
    }

    void testProjectWithNoMesh() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        ProjectData data;
        data.scaleFactor = 1.0f;
        QVERIFY(ProjectFile::save(QDir(tempDir.path()).filePath("empty.room"), data));

        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--project", QDir(tempDir.path()).filePath("empty.room")});
        proc.start();
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitCode(), 1);
    }

    // -- Happy-path tests ----------------------------------------------------

    void testCliRendersFixtureProject() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString projectDir = tempDir.path();
        writeSquarePlaneObj(QDir(projectDir).filePath("model.obj"));
        writeFixtureAudio(QDir(projectDir).filePath("source.wav"));

        ProjectData project = makeProject();
        QVERIFY(ProjectFile::save(QDir(projectDir).filePath("fixture.room"), project));

        const QString outputDir = QDir(projectDir).filePath("out");
        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--project", QDir(projectDir).filePath("fixture.room"), "--output", outputDir, "--method",
                           "ray", "--max-order", "1", "--n-rays", "128", "--scattering", "0.1", "--air-absorption",
                           "off", "--sample-rate", "22050"});
        proc.setWorkingDirectory(projectDir);
        proc.start();
        QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
        QVERIFY2(proc.waitForFinished(120000), "CLI render timed out");
        QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
        QCOMPARE(proc.exitCode(), 0);

        QFileInfo outInfo(outputDir);
        QVERIFY(outInfo.exists());
        QVERIFY(outInfo.isDir());

        QDir outDir(outputDir);
        QVERIFY(outDir.exists("scene.json"));
        QVERIFY(outDir.exists("metrics.json"));
        QVERIFY(outDir.exists("metrics.csv"));
        QVERIFY(outDir.exists("summary.json"));

        const QStringList wavs = outDir.entryList(QStringList() << "*.wav", QDir::Files);
        QVERIFY2(!wavs.isEmpty(), "Expected at least one WAV output");

        QFile metricsFile(outDir.filePath("metrics.json"));
        QVERIFY(metricsFile.open(QIODevice::ReadOnly));
        const QJsonDocument metricsDoc = QJsonDocument::fromJson(metricsFile.readAll());
        metricsFile.close();
        QVERIFY(metricsDoc.isObject());
        QCOMPARE(metricsDoc.object().value("sample_rate").toInt(), 22050);
        QVERIFY(metricsDoc.object().value("pairs").isArray());
        QCOMPARE(metricsDoc.object().value("pairs").toArray().size(), 1);

        QFile summaryFile(outDir.filePath("summary.json"));
        QVERIFY(summaryFile.open(QIODevice::ReadOnly));
        const QJsonDocument summaryDoc = QJsonDocument::fromJson(summaryFile.readAll());
        summaryFile.close();
        QVERIFY(summaryDoc.isObject());
        QCOMPARE(summaryDoc.object().value("sample_rate").toInt(), 22050);

        QFile csvFile(outDir.filePath("metrics.csv"));
        QVERIFY(csvFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QByteArray csvContents = csvFile.readAll();
        csvFile.close();
        QVERIFY(csvContents.startsWith("source,listener"));
    }

    void testVersionFlag() {
        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--version"});
        proc.start();
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitCode(), 0);
        const QByteArray output = proc.readAllStandardOutput();
        QVERIFY2(output.contains("1.0.0"), qPrintable(QString("Unexpected version output: %1").arg(QString(output))));
    }

    void testSampleRateOverride() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString projectDir = tempDir.path();
        writeSquarePlaneObj(QDir(projectDir).filePath("model.obj"));
        writeFixtureAudio(QDir(projectDir).filePath("source.wav"));
        QVERIFY(ProjectFile::save(QDir(projectDir).filePath("fixture.room"), makeProject()));

        const QString outputDir = QDir(projectDir).filePath("out");
        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--project", QDir(projectDir).filePath("fixture.room"), "--output", outputDir,
                           "--max-order", "1", "--n-rays", "64", "--sample-rate", "16000"});
        proc.setWorkingDirectory(projectDir);
        proc.start();
        QVERIFY(proc.waitForStarted());
        QVERIFY2(proc.waitForFinished(120000), "CLI render timed out");
        QCOMPARE(proc.exitCode(), 0);

        QFile metricsFile(QDir(outputDir).filePath("metrics.json"));
        QVERIFY(metricsFile.open(QIODevice::ReadOnly));
        const QJsonDocument doc = QJsonDocument::fromJson(metricsFile.readAll());
        metricsFile.close();
        QCOMPARE(doc.object().value("sample_rate").toInt(), 16000);
    }

    void testMetricsVersionConsistency() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString projectDir = tempDir.path();
        writeSquarePlaneObj(QDir(projectDir).filePath("model.obj"));
        writeFixtureAudio(QDir(projectDir).filePath("source.wav"));
        QVERIFY(ProjectFile::save(QDir(projectDir).filePath("fixture.room"), makeProject()));

        const QString outputDir = QDir(projectDir).filePath("out");
        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--project", QDir(projectDir).filePath("fixture.room"), "--output", outputDir,
                           "--max-order", "1", "--n-rays", "64"});
        proc.setWorkingDirectory(projectDir);
        proc.start();
        QVERIFY(proc.waitForStarted());
        QVERIFY2(proc.waitForFinished(120000), "CLI render timed out");
        QCOMPARE(proc.exitCode(), 0);

        QFile metricsFile(QDir(outputDir).filePath("metrics.json"));
        QVERIFY(metricsFile.open(QIODevice::ReadOnly));
        const QJsonDocument metricsDoc = QJsonDocument::fromJson(metricsFile.readAll());
        metricsFile.close();
        const QString metricsVersion = metricsDoc.object().value("version").toString();

        QFile summaryFile(QDir(outputDir).filePath("summary.json"));
        QVERIFY(summaryFile.open(QIODevice::ReadOnly));
        const QJsonDocument summaryDoc = QJsonDocument::fromJson(summaryFile.readAll());
        summaryFile.close();
        const QString summaryVersion = summaryDoc.object().value("version").toString();

        QCOMPARE(metricsVersion, summaryVersion);
    }

    void testCsvRowCount() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString projectDir = tempDir.path();
        writeSquarePlaneObj(QDir(projectDir).filePath("model.obj"));
        writeFixtureAudio(QDir(projectDir).filePath("source.wav"));
        QVERIFY(ProjectFile::save(QDir(projectDir).filePath("fixture.room"), makeProject()));

        const QString outputDir = QDir(projectDir).filePath("out");
        QProcess proc;
        proc.setProgram(cliPath_);
        proc.setArguments({"--project", QDir(projectDir).filePath("fixture.room"), "--output", outputDir,
                           "--max-order", "1", "--n-rays", "64"});
        proc.setWorkingDirectory(projectDir);
        proc.start();
        QVERIFY(proc.waitForStarted());
        QVERIFY2(proc.waitForFinished(120000), "CLI render timed out");
        QCOMPARE(proc.exitCode(), 0);

        QFile csvFile(QDir(outputDir).filePath("metrics.csv"));
        QVERIFY(csvFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QList<QByteArray> lines = csvFile.readAll().split('\n');
        csvFile.close();

        QVERIFY(lines.at(0).startsWith("source,listener"));
        int dataRows = 0;
        for (int i = 1; i < lines.size(); ++i)
            if (!lines.at(i).trimmed().isEmpty())
                ++dataRows;
        QCOMPARE(dataRows, 1);
    }

  private:
    QString cliPath_;
};

QTEST_MAIN(TestRenderCli)
#include "test_render_cli.moc"
