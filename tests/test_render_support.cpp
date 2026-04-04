#include "acoustics/RenderExports.h"
#include "acoustics/RenderOptions.h"
#include "acoustics/RenderPipeline.h"
#include "acoustics/SimulationQueue.h"
#include "audio/AudioFile.h"
#include "core/PlacedPoint.h"
#include "core/ProjectFile.h"
#include "rendering/Camera.h"
#include "rendering/RayPicking.h"
#include "rendering/TextureManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSignalSpy>
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

void writeAudio(const QString& path) {
    AudioFile af;
    af.samples() = {0.0f, 0.5f, 0.0f, -0.5f};
    QVERIFY(af.save(path, 22050));
}

ProjectData makeProject() {
    ProjectData data;
    data.stlFilePath = "model.obj";
    data.scaleFactor = 1.0f;
    data.sampleRate = 22050;
    data.soundSourceFile = "source.wav";
    data.surfaceColors = {{0.2f, 0.3f, 0.4f}};
    data.surfaceMaterials = {std::nullopt};

    PlacedPoint source;
    source.surfacePoint = Vec3f(0.25f, 0.25f, 0.0f);
    source.normal = Vec3f(0.0f, 0.0f, 1.0f);
    source.distance = 0.1f;
    source.pointType = POINT_TYPE_SOURCE;
    source.name = "Source";
    data.placedPoints.push_back(source);

    PlacedPoint listener;
    listener.surfacePoint = Vec3f(0.75f, 0.75f, 0.0f);
    listener.normal = Vec3f(0.0f, 0.0f, 1.0f);
    listener.distance = 0.1f;
    listener.pointType = POINT_TYPE_LISTENER;
    listener.name = "Listener";
    listener.orientationYaw = 45.0f;
    data.placedPoints.push_back(listener);

    return data;
}

} // namespace

class TestRenderSupport : public QObject {
    Q_OBJECT
  private slots:
    void testRenderExportsWriteFiles() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString monoPath = dir.filePath("mono.wav");
        const QString stereoPath = dir.filePath("stereo.wav");
        const QString jsonPath = dir.filePath("summary.json");
        const QString csvPath = dir.filePath("metrics.csv");

        QVERIFY(RenderExports::saveMonoWav(monoPath, 22050, {0.0f, 0.25f, -0.25f}));
        QVERIFY(RenderExports::saveStereoWav(stereoPath, 22050, {0.0f, 0.1f}, {0.0f, -0.1f}));

        QJsonArray pairs;
        QJsonObject pair;
        pair["source"] = "Source";
        pair["listener"] = "Listener";
        pair["reverberation_time"] = QJsonObject{{"T20", 1.1}, {"T30", 1.5}, {"EDT", 0.9}};
        pair["sound_pressure_level"] = QJsonObject{{"direct_dB", 10.0}, {"reflected_dB", 8.0}, {"total_dB", 11.0}};
        pairs.append(pair);

        QVERIFY(RenderExports::saveJsonObject(jsonPath, RenderExports::buildMetricsSummary(pairs, 22050)));
        QVERIFY(RenderExports::saveMetricsCsv(csvPath, pairs));

        AudioFile loaded;
        QVERIFY(loaded.load(monoPath));
        QCOMPARE(loaded.sampleRate(), 22050);
        QVERIFY(!loaded.samples().empty());

        QFile jsonFile(jsonPath);
        QVERIFY(jsonFile.open(QIODevice::ReadOnly));
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
        jsonFile.close();
        QCOMPARE(jsonDoc.object().value("sample_rate").toInt(), 22050);

        QFile csvFile(csvPath);
        QVERIFY(csvFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QByteArray csv = csvFile.readAll();
        csvFile.close();
        QVERIFY(csv.startsWith("source,listener"));
    }

    void testRenderPipelineBuildsParams() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        writeSquarePlaneObj(dir.filePath("model.obj"));
        writeAudio(dir.filePath("source.wav"));

        ProjectData project = makeProject();
        QVERIFY(ProjectFile::save(dir.filePath("fixture.room"), project));

        RenderOptions options;
        options.sampleRate = 22050;
        options.maxOrder = 2;
        options.nRays = 128;
        options.scattering = 0.15f;
        options.airAbsorption = false;
        options.method = RenderMethod::DG_2D;
        options.dgPolyOrder = 4;
        options.dgMaxFrequency = 1800.0f;

        SimulationWorker::Params params;
        QString error;
        QVERIFY(RenderPipeline::buildSimulationParams(dir.filePath("fixture.room"), project, options, {1},
                                                      dir.filePath("out"), &params, &error));
        QCOMPARE(params.sampleRate, 22050);
        QCOMPARE(params.maxOrder, 2);
        QCOMPARE(params.nRays, 128);
        QCOMPARE(params.scattering, 0.15f);
        QCOMPARE(params.airAbsorption, false);
        QCOMPARE(params.method, SimMethod::DG_2D);
        QCOMPARE(params.dgPolyOrder, 4);
        QCOMPARE(params.dgMaxFrequency, 1800.0f);
        QCOMPARE(params.outputDir, dir.filePath("out"));
        QCOMPARE(params.scene.soundSourceCount(), 1);
        QCOMPARE(params.scene.listenerCount(), 1);
        QCOMPARE(static_cast<int>(params.walls.size()), 1);
        QVERIFY(params.modelVertices.size() >= 6);
    }

    void testSimulationQueueReportsFailedEmptyJob() {
        SimulationQueue queue;
        QSignalSpy errorSpy(&queue, &SimulationQueue::jobError);
        QVERIFY(errorSpy.isValid());

        SimulationWorker::Params params;
        const int jobId = queue.enqueue(params, "empty");
        QCOMPARE(jobId, 1);
        QVERIFY(errorSpy.wait(5000));
        QCOMPARE(queue.queueSize(), 0);
        QVERIFY(!queue.isRunning());
        const auto jobs = queue.allJobs();
        QCOMPARE(static_cast<int>(jobs.size()), 1);
        QCOMPARE(jobs[0].id, 1);
        QCOMPARE(jobs[0].status, SimulationQueue::JobStatus::Failed);
        QVERIFY(jobs[0].errorMessage.contains("Need at least 1 source and 1 listener"));
    }

    void testCameraAndRayPicking() {
        Camera cam;
        cam.reset(10.0f);
        cam.orbit(10.0f, -400.0f);
        cam.zoom(2.0f, 10.0f);
        Vec3f eye = cam.eyePosition();
        QVERIFY(std::isfinite(eye.x()));
        QVERIFY(std::isfinite(eye.y()));
        QVERIFY(std::isfinite(eye.z()));

        Triangle tri;
        tri.v0 = Vec3f(0.0f, 0.0f, 0.0f);
        tri.v1 = Vec3f(1.0f, 0.0f, 0.0f);
        tri.v2 = Vec3f(0.0f, 1.0f, 0.0f);
        tri.normal = Vec3f(0.0f, 0.0f, 1.0f);

        auto hit = RayPicking::rayTriangleIntersect(Vec3f(0.25f, 0.25f, 1.0f), Vec3f(0.0f, 0.0f, -1.0f), tri);
        QVERIFY(hit.has_value());

        auto sphereHit = RayPicking::raySphereIntersect(Vec3f(-2.0f, 0.0f, 0.0f), Vec3f(1.0f, 0.0f, 0.0f),
                                                        Vec3f(0.0f, 0.0f, 0.0f), 1.0f);
        QVERIFY(sphereHit.has_value());

        QVERIFY(RayPicking::segmentPassesThroughSphere(Vec3f(-2.0f, 0.0f, 0.0f), Vec3f(2.0f, 0.0f, 0.0f),
                                                       Vec3f(0.0f, 0.0f, 0.0f), 1.0f));
    }

    void testTextureManagerHandlesMissingTextures() {
        TextureManager textures;
        QCOMPARE(textures.getOrLoad(QString()), 0u);
        QVERIFY(!textures.has("missing.png"));
        QCOMPARE(textures.getOrLoad("missing.png"), 0u);
        QVERIFY(!textures.has("missing.png"));
        textures.release("missing.png");
        textures.releaseAll();
    }

    // --- Phase 3.3: binaural export modes and naming ---

    void testBinauralOutputPathLayoutAndSanitisation() {
        // Spaces in names must be replaced with underscores.
        const QString path = RenderExports::binauralListenerOutputPath(
            "/out", "Main Listener", "Source A");
        QVERIFY(path.contains("binaural"));
        QVERIFY(path.contains("Main_Listener"));
        QVERIFY(path.contains("Source_A.wav"));
        // Must be under outputDir
        QVERIFY(path.startsWith("/out"));
    }

    void testBinauralOutputPathDistinctListeners() {
        const QString p1 = RenderExports::binauralListenerOutputPath("/o", "L1", "S1");
        const QString p2 = RenderExports::binauralListenerOutputPath("/o", "L2", "S1");
        QVERIFY(p1 != p2);
    }

    void testAddHrtfMetadataWithExplicitPath() {
        QJsonObject obj;
        obj["sample_rate"] = 48000;
        RenderExports::addHrtfMetadata(obj, "/datasets/kemarSmall.sofa");
        QVERIFY(obj.contains("hrtf"));
        const QJsonObject hrtf = obj.value("hrtf").toObject();
        QCOMPARE(hrtf.value("output_mode").toString(), QStringLiteral("binaural"));
        QCOMPARE(hrtf.value("dataset_path").toString(), QStringLiteral("/datasets/kemarSmall.sofa"));
        // Original fields must be preserved
        QCOMPARE(obj.value("sample_rate").toInt(), 48000);
    }

    void testAddHrtfMetadataEmptyPathYieldsSynthetic() {
        QJsonObject obj;
        RenderExports::addHrtfMetadata(obj, {});
        QCOMPARE(obj.value("hrtf").toObject().value("dataset_path").toString(),
                 QStringLiteral("synthetic"));
    }

    void testRenderOptionsBinauralFieldsFlowThroughPipeline() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        writeSquarePlaneObj(dir.filePath("model.obj"));
        writeAudio(dir.filePath("source.wav"));
        ProjectData project = makeProject();
        QVERIFY(ProjectFile::save(dir.filePath("fixture.room"), project));

        RenderOptions options;
        options.sampleRate = 22050;
        options.outputMode = AudioOutputMode::Binaural;
        options.hrtfDatasetPath = "/tmp/test.sofa";

        SimulationWorker::Params params;
        QString error;
        QVERIFY(RenderPipeline::buildSimulationParams(dir.filePath("fixture.room"), project, options, {},
                                                      dir.filePath("out"), &params, &error));
        QCOMPARE(params.outputMode, AudioOutputMode::Binaural);
        QCOMPARE(params.hrtfDatasetPath, QStringLiteral("/tmp/test.sofa"));
    }

    void testStereoWavRoundTripPreservesSampleRate() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const std::vector<float> left  = {0.1f, 0.2f, -0.1f, -0.2f};
        const std::vector<float> right = {-0.1f, -0.2f, 0.1f, 0.2f};
        const QString path = dir.filePath("binaural.wav");
        QVERIFY(RenderExports::saveStereoWav(path, 48000, left, right));

        AudioFile loaded;
        QVERIFY(loaded.load(path));
        QCOMPARE(loaded.sampleRate(), 48000);
        // AudioFile loads stereo as a single (mixed/first-channel) buffer;
        // the important thing is the file round-trips with the correct sample rate
        // and a non-empty sample buffer.
        QVERIFY(!loaded.samples().empty());
    }
};

QTEST_MAIN(TestRenderSupport)
#include "test_render_support.moc"
