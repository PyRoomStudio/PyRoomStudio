#include "acoustics/HrtfDataset.h"

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
};

QTEST_MAIN(TestHrtf)
#include "test_hrtf.moc"
