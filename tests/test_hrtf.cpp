#include "acoustics/HrtfDataset.h"
#include "acoustics/HrtfLookup.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace prs;

namespace {

void expectSamples(const HrtfSample& sample, const std::vector<float>& left, const std::vector<float>& right,
                   int leftDelay, int rightDelay) {
    QCOMPARE(sample.left, left);
    QCOMPARE(sample.right, right);
    QCOMPARE(sample.leftDelaySamples, leftDelay);
    QCOMPARE(sample.rightDelaySamples, rightDelay);
}

} // namespace

class TestHrtf : public QObject {
    Q_OBJECT
  private slots:
    void testSyntheticDatasetMetadataAndDirectionalLookup() {
        auto dataset = makeSyntheticHrtfDataset();
        QVERIFY(dataset);

        QString error;
        QVERIFY2(dataset->validate(&error), qPrintable(error));

        const HrtfMetadata metadata = dataset->metadata();
        QCOMPARE(metadata.format, QStringLiteral("synthetic"));
        QCOMPARE(metadata.sampleRate, 48000);
        QCOMPARE(metadata.filterLength, 3);
        QVERIFY(metadata.normalized);

        auto front = dataset->lookup(0.0f, 0.0f);
        QVERIFY(front.has_value());
        expectSamples(*front, {1.0f, 0.0f, 0.0f}, {0.8f, 0.0f, 0.0f}, 0, 0);

        auto right = dataset->lookup(90.0f, 0.0f);
        QVERIFY(right.has_value());
        expectSamples(*right, {0.2f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0, 1);
    }

    void testInMemoryDatasetValidationRejectsInvalidFilterLengths() {
        HrtfMetadata metadata;
        metadata.name = "broken";
        metadata.format = "memory";
        metadata.sampleRate = 44100;
        metadata.filterLength = 4;
        metadata.normalized = true;

        InMemoryHrtfDataset dataset(metadata, {HrtfSample{0.0f, 0.0f, 0, 0, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                                               HrtfSample{90.0f, 0.0f, 0, 0, {1.0f, 0.0f}, {1.0f, 0.0f}}});

        QString error;
        QVERIFY(!dataset.validate(&error));
        QVERIFY(error.contains("filter length"));
    }

    void testSofaLoaderFailsCleanlyWhenBackendUnavailableOrFileMissing() {
        QString error;
        auto dataset = loadSofaHrtfDataset(QTemporaryDir().filePath("missing.sofa"), 48000, &error);
        QVERIFY(!dataset);
        QVERIFY(!error.isEmpty());
    }

    // --- Phase 3.2: head-frame direction transform ---

    void testToHeadFrameSourceDirectlyAhead() {
        // Listener at origin facing +X; source is also along +X → azimuth 0, elevation 0.
        const HeadFrameDirection dir =
            toHeadFrame(Vec3f{1.0f, 0.0f, 0.0f}, Vec3f::Zero(), Vec3f{1.0f, 0.0f, 0.0f});
        QCOMPARE(qRound(dir.azimuthDeg), 0);
        QCOMPARE(qRound(dir.elevationDeg), 0);
    }

    void testToHeadFrameSourceToListenerRight() {
        // Listener at origin facing +X with Y-up.
        // right = forward × worldUp = (1,0,0) × (0,1,0) = (0,0,1).
        // Source at (0,0,1) → azimuth = +90°.
        const HeadFrameDirection dir =
            toHeadFrame(Vec3f{0.0f, 0.0f, 1.0f}, Vec3f::Zero(), Vec3f{1.0f, 0.0f, 0.0f});
        QCOMPARE(qRound(dir.azimuthDeg), 90);
        QCOMPARE(qRound(dir.elevationDeg), 0);
    }

    void testToHeadFrameSourceToListenerLeft() {
        // Source at (0,0,-1) → azimuth = -90°.
        const HeadFrameDirection dir =
            toHeadFrame(Vec3f{0.0f, 0.0f, -1.0f}, Vec3f::Zero(), Vec3f{1.0f, 0.0f, 0.0f});
        QCOMPARE(qRound(dir.azimuthDeg), -90);
        QCOMPARE(qRound(dir.elevationDeg), 0);
    }

    void testToHeadFrameSourceAboveListener() {
        // Source directly above listener → elevation = +90°.
        const HeadFrameDirection dir =
            toHeadFrame(Vec3f{0.0f, 1.0f, 0.0f}, Vec3f::Zero(), Vec3f{1.0f, 0.0f, 0.0f});
        QCOMPARE(qRound(dir.elevationDeg), 90);
    }

    void testListenerOrientationChangesLRResponse() {
        // Source fixed at world (0,0,1).  When the listener faces +Z the source
        // is straight ahead (azimuth ≈ 0°).  When the listener faces +X the source
        // is to the listener's right (azimuth ≈ 90°).
        // The synthetic dataset has distinct L/R levels for front vs. right, so
        // the two orientations must produce different left channel amplitudes.
        auto dataset = makeSyntheticHrtfDataset();
        const Vec3f sourcePos{0.0f, 0.0f, 1.0f};
        const Vec3f listenerPos = Vec3f::Zero();

        // Listener facing toward source (+Z)
        const auto dirFacing = toHeadFrame(sourcePos, listenerPos, Vec3f{0.0f, 0.0f, 1.0f});
        const HrtfSample facingSample = lookupWithFallback(*dataset, dirFacing.azimuthDeg, dirFacing.elevationDeg);

        // Listener facing +X (source is to the right)
        const auto dirTurned = toHeadFrame(sourcePos, listenerPos, Vec3f{1.0f, 0.0f, 0.0f});
        const HrtfSample turnedSample = lookupWithFallback(*dataset, dirTurned.azimuthDeg, dirTurned.elevationDeg);

        // The left channel amplitudes must differ between the two orientations.
        QVERIFY(!facingSample.left.empty());
        QVERIFY(!turnedSample.left.empty());
        QVERIFY(facingSample.left != turnedSample.left);
    }

    void testLookupWithFallbackReturnsImpulseForEmptyDataset() {
        // An InMemoryHrtfDataset with no samples cannot satisfy any lookup; the
        // fallback must supply a unit-impulse passthrough.
        HrtfMetadata meta;
        meta.name = "empty";
        meta.format = "memory";
        meta.sampleRate = 48000;
        meta.filterLength = 4;
        meta.normalized = false;
        InMemoryHrtfDataset empty(meta, {});

        const HrtfSample sample = lookupWithFallback(empty, 0.0f, 0.0f);
        QCOMPARE(static_cast<int>(sample.left.size()), 4);
        QCOMPARE(static_cast<int>(sample.right.size()), 4);
        QCOMPARE(sample.left[0], 1.0f);
        QCOMPARE(sample.right[0], 1.0f);
        QCOMPARE(sample.leftDelaySamples, 0);
        QCOMPARE(sample.rightDelaySamples, 0);
    }
};

QTEST_MAIN(TestHrtf)
#include "test_hrtf.moc"
