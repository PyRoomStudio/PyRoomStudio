#include <QtTest/QtTest>
#include <QTemporaryFile>
#include "audio/AudioFile.h"
#include "audio/SignalProcessing.h"

using namespace prs;

class TestAudio : public QObject {
    Q_OBJECT
private slots:
    void testAudioFileSaveAndLoad() {
        AudioFile af;
        std::vector<float> samples(1000);
        for (int i = 0; i < 1000; ++i)
            samples[i] = static_cast<float>(i) / 1000.0f;
        af.samples() = samples;

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        tmp.setFileTemplate(QDir::tempPath() + "/testXXXXXX.wav");
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(af.save(path, 44100));

        AudioFile loaded;
        QVERIFY(loaded.load(path));
        QCOMPARE(loaded.sampleRate(), 44100);
        QVERIFY(loaded.samples().size() == 1000);
    }

    void testMonoConversion() {
        AudioFile af;
        std::vector<float> stereo(200);
        for (int i = 0; i < 200; ++i)
            stereo[i] = (i % 2 == 0) ? 1.0f : 0.0f;
        af.samples() = stereo;
        af.convertToMono();
        QVERIFY(!af.samples().empty());
    }

    void testVolumeApplication() {
        AudioFile af;
        af.samples() = {0.5f, -0.5f, 1.0f};
        af.applyVolume(0.5f);
        QVERIFY(std::abs(af.samples()[0] - 0.25f) < 1e-6f);
        QVERIFY(std::abs(af.samples()[1] + 0.25f) < 1e-6f);
    }

    void testNormalize() {
        std::vector<float> signal = {0.2f, -0.5f, 0.3f};
        SignalProcessing::normalize(signal, 0.95f);
        float maxAbs = 0;
        for (float s : signal) maxAbs = std::max(maxAbs, std::abs(s));
        QVERIFY(std::abs(maxAbs - 0.95f) < 1e-5f);
    }

    void testConvolve() {
        std::vector<float> a = {1, 0, 0};
        std::vector<float> b = {1, 2, 3};
        auto result = SignalProcessing::convolve(a, b);
        QCOMPARE(result.size(), static_cast<size_t>(5));
        QVERIFY(std::abs(result[0] - 1.0f) < 1e-5f);
        QVERIFY(std::abs(result[1] - 2.0f) < 1e-5f);
        QVERIFY(std::abs(result[2] - 3.0f) < 1e-5f);
    }

    void testFftConvolve() {
        std::vector<float> a = {1, 0, 0, 0};
        std::vector<float> b = {1, 2, 3, 4};
        auto result = SignalProcessing::fftConvolve(a, b);
        QVERIFY(result.size() >= 4);
        QVERIFY(std::abs(result[0] - 1.0f) < 1e-4f);
        QVERIFY(std::abs(result[1] - 2.0f) < 1e-4f);
        QVERIFY(std::abs(result[2] - 3.0f) < 1e-4f);
        QVERIFY(std::abs(result[3] - 4.0f) < 1e-4f);
    }
};

QTEST_MAIN(TestAudio)
#include "test_audio.moc"
