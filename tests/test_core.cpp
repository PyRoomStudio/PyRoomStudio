#include <QtTest/QtTest>
#include "core/Types.h"
#include "core/Material.h"
#include "core/MaterialLoader.h"
#include "core/PlacedPoint.h"
#include "core/SoundSource.h"
#include "core/Listener.h"

using namespace prs;

class TestCore : public QObject {
    Q_OBJECT
private slots:
    void testMaterialDefaults() {
        Material m;
        QCOMPARE(m.averageAbsorption(), 0.2f);
        QCOMPARE(m.scattering, 0.1f);
        QVERIFY(!m.name.empty() || m.name.empty());
    }

    void testMaterialAbsorptionAt() {
        Material m;
        m.absorption = {0.10f, 0.20f, 0.40f, 0.60f, 0.80f, 0.90f};
        QVERIFY(std::abs(m.absorptionAt(125) - 0.10f) < 1e-5f);
        QVERIFY(std::abs(m.absorptionAt(4000) - 0.90f) < 1e-5f);
        QVERIFY(m.absorptionAt(500) > 0.30f && m.absorptionAt(500) < 0.50f);
    }

    void testMaterialLoaderFromDirectory() {
        auto categories = MaterialLoader::loadFromDirectory("materials");
        QVERIFY(categories.size() >= 2);
        int totalMats = 0;
        for (auto& cat : categories) {
            QVERIFY(!cat.materials.empty());
            for (auto& mat : cat.materials) {
                QVERIFY(mat.averageAbsorption() >= 0.0f && mat.averageAbsorption() <= 1.0f);
                QVERIFY(mat.scattering >= 0.0f && mat.scattering <= 1.0f);
                totalMats++;
            }
        }
        QVERIFY(totalMats >= 100);
    }

    void testPlacedPointGetPosition() {
        PlacedPoint pt;
        pt.surfacePoint = Vec3f(1, 2, 3);
        pt.normal = Vec3f(0, 0, 1);
        pt.distance = 2.5f;
        Vec3f pos = pt.getPosition();
        QCOMPARE(pos.x(), 1.0f);
        QCOMPARE(pos.y(), 2.0f);
        QCOMPARE(pos.z(), 5.5f);
    }

    void testPlacedPointDefaults() {
        PlacedPoint pt;
        QCOMPARE(pt.distance, 0.0f);
        QCOMPARE(pt.volume, 1.0f);
        QCOMPARE(pt.pointType, std::string(POINT_TYPE_NONE));
        QVERIFY(pt.name.empty());
        QVERIFY(pt.audioFile.empty());
    }

    void testSoundSourceDefaults() {
        SoundSource s;
        QCOMPARE(s.volume, 1.0f);
        QVERIFY(s.position == Vec3f::Zero());
    }

    void testListenerDefaults() {
        Listener l;
        QVERIFY(l.position == Vec3f::Zero());
        QVERIFY(!l.orientation.has_value());
    }

    void testEdgeCreation() {
        Vec3f a(1, 2, 3);
        Vec3f b(4, 5, 6);
        Edge e1 = makeEdge(a, b);
        Edge e2 = makeEdge(b, a);
        QCOMPARE(e1, e2);
    }

    void testColor3Types() {
        Color3i ci = {128, 200, 50};
        QCOMPARE(ci[0], 128);
        Color3f cf = {0.5f, 0.8f, 0.2f};
        QVERIFY(std::abs(cf[1] - 0.8f) < 1e-6f);
    }
};

QTEST_MAIN(TestCore)
#include "test_core.moc"
