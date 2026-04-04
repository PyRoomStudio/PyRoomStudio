#include "scene/SceneManager.h"

#include <QTemporaryFile>
#include <QtTest/QtTest>

using namespace prs;

class TestSceneManager : public QObject {
    Q_OBJECT
  private slots:
    void testAddRemoveSources() {
        SceneManager sm;
        sm.addSoundSource(Vec3f(1, 2, 3), "test.wav", 0.8f, "Src1");
        QCOMPARE(sm.soundSourceCount(), 1);
        QCOMPARE(sm.getSoundSource(0)->name, std::string("Src1"));
        QCOMPARE(sm.getSoundSource(0)->volume, 0.8f);

        sm.addSoundSource(Vec3f(4, 5, 6), "test2.wav");
        QCOMPARE(sm.soundSourceCount(), 2);

        QVERIFY(sm.removeSoundSource(0));
        QCOMPARE(sm.soundSourceCount(), 1);
    }

    void testAddRemoveListeners() {
        SceneManager sm;
        sm.addListener(Vec3f(0, 0, 0), "L1");
        sm.addListener(Vec3f(1, 1, 1), "L2");
        QCOMPARE(sm.listenerCount(), 2);
        QCOMPARE(sm.getListener(1)->name, std::string("L2"));

        QVERIFY(sm.removeListener(0));
        QCOMPARE(sm.listenerCount(), 1);
    }

    void testClearAll() {
        SceneManager sm;
        sm.addSoundSource(Vec3f::Zero(), "a.wav");
        sm.addListener(Vec3f::Zero(), "L");
        sm.clearAll();
        QCOMPARE(sm.soundSourceCount(), 0);
        QCOMPARE(sm.listenerCount(), 0);
    }

    void testHasMinimumObjects() {
        SceneManager sm;
        QVERIFY(!sm.hasMinimumObjects());
        sm.addSoundSource(Vec3f::Zero(), "a.wav");
        QVERIFY(!sm.hasMinimumObjects());
        sm.addListener(Vec3f::Zero(), "L");
        QVERIFY(sm.hasMinimumObjects());
    }

    void testSelection() {
        SceneManager sm;
        sm.addSoundSource(Vec3f::Zero(), "a.wav");
        sm.addListener(Vec3f::Zero(), "L");

        sm.selectSource(0);
        auto sel = sm.getSelectedObject();
        QVERIFY(sel.has_value());
        QCOMPARE(sel->first, std::string("source"));

        sm.selectListener(0);
        sel = sm.getSelectedObject();
        QCOMPARE(sel->first, std::string("listener"));

        sm.clearSelection();
        QVERIFY(!sm.getSelectedObject().has_value());
    }

    void testJsonSaveLoad() {
        SceneManager sm;
        sm.addSoundSource(Vec3f(1, 2, 3), "audio.wav", 0.5f, "S");
        sm.addListener(Vec3f(4, 5, 6), "L");

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        sm.saveToFile(path);

        SceneManager loaded;
        loaded.loadFromFile(path);
        QCOMPARE(loaded.soundSourceCount(), 1);
        QCOMPARE(loaded.listenerCount(), 1);
        QCOMPARE(loaded.getSoundSource(0)->name, std::string("S"));
        QVERIFY(std::abs(loaded.getSoundSource(0)->volume - 0.5f) < 1e-6f);
    }

    void testSummary() {
        SceneManager sm;
        sm.addSoundSource(Vec3f::Zero(), "a.wav", 1.0f, "S");
        sm.addListener(Vec3f::Zero(), "L");
        QString summary = sm.getSummary();
        QVERIFY(summary.contains("1 sound source"));
        QVERIFY(summary.contains("1 listener"));
    }

    void testBoundsChecks() {
        SceneManager sm;
        QVERIFY(!sm.removeSoundSource(0));
        QVERIFY(!sm.removeListener(0));
        QVERIFY(sm.getSoundSource(0) == nullptr);
        QVERIFY(sm.getListener(-1) == nullptr);
    }
};

QTEST_MAIN(TestSceneManager)
#include "test_scene_manager.moc"
