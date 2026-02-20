#include <QtTest/QtTest>

#include "acoustics/ImageSourceMethod.h"
#include "acoustics/RayTracer.h"
#include "acoustics/RoomImpulseResponse.h"
#include "acoustics/Wall.h"
#include "audio/SignalProcessing.h"
#include "rendering/RayPicking.h"

using namespace prs;

class TestAcousticSimulation : public QObject {
    Q_OBJECT

private:
    std::vector<Wall> createBoxRoom(float sizeX, float sizeY, float sizeZ) {
        std::vector<Wall> walls;
        auto addQuad = [&](Vec3f a, Vec3f b, Vec3f c, Vec3f d) {
            Wall w1, w2;
            Vec3f e1 = b - a, e2 = c - a;
            Vec3f n = e1.cross(e2).normalized();
            w1.triangle = {a, b, c, n};
            w1.energyAbsorption = 0.2f;
            w1.scattering = 0.1f;
            walls.push_back(w1);

            Vec3f e3 = d - a, e4 = c - a;
            Vec3f n2 = e4.cross(e3).normalized();
            w2.triangle = {a, c, d, n2};
            w2.energyAbsorption = 0.2f;
            w2.scattering = 0.1f;
            walls.push_back(w2);
        };

        float hx = sizeX / 2, hy = sizeY / 2, hz = sizeZ / 2;
        // Floor (z = -hz)
        addQuad({-hx,-hy,-hz}, {hx,-hy,-hz}, {hx,hy,-hz}, {-hx,hy,-hz});
        // Ceiling (z = hz)
        addQuad({-hx,-hy,hz}, {-hx,hy,hz}, {hx,hy,hz}, {hx,-hy,hz});
        // Front (y = -hy)
        addQuad({-hx,-hy,-hz}, {-hx,-hy,hz}, {hx,-hy,hz}, {hx,-hy,-hz});
        // Back (y = hy)
        addQuad({-hx,hy,-hz}, {hx,hy,-hz}, {hx,hy,hz}, {-hx,hy,hz});
        // Left (x = -hx)
        addQuad({-hx,-hy,-hz}, {-hx,hy,-hz}, {-hx,hy,hz}, {-hx,-hy,hz});
        // Right (x = hx)
        addQuad({hx,-hy,-hz}, {hx,-hy,hz}, {hx,hy,hz}, {hx,hy,-hz});

        return walls;
    }

private slots:
    void testRayTriangleIntersect() {
        Triangle tri;
        tri.v0 = {0, 0, 0};
        tri.v1 = {1, 0, 0};
        tri.v2 = {0, 1, 0};
        tri.normal = {0, 0, 1};

        // Hit
        auto t = RayPicking::rayTriangleIntersect({0.2f, 0.2f, 1.0f}, {0, 0, -1}, tri);
        QVERIFY(t.has_value());
        QVERIFY(std::abs(*t - 1.0f) < 0.01f);

        // Miss
        auto t2 = RayPicking::rayTriangleIntersect({5.0f, 5.0f, 1.0f}, {0, 0, -1}, tri);
        QVERIFY(!t2.has_value());
    }

    void testRaySphereIntersect() {
        auto t = RayPicking::raySphereIntersect({0, 0, 5}, {0, 0, -1}, {0, 0, 0}, 1.0f);
        QVERIFY(t.has_value());
        QVERIFY(std::abs(*t - 4.0f) < 0.01f);
    }

    void testImageSourceMethod() {
        auto walls = createBoxRoom(6.0f, 6.0f, 3.0f);
        Vec3f source(1, 1, 0);
        Vec3f listener(-1, -1, 0);

        ImageSourceMethod ism;
        auto results = ism.compute(source, listener, walls, 2);

        QVERIFY(!results.empty());
        // At minimum, direct sound should be present
        bool hasDirect = false;
        for (auto& is : results) {
            if (is.order == 0) { hasDirect = true; break; }
        }
        QVERIFY(hasDirect);
    }

    void testRayTracer() {
        auto walls = createBoxRoom(6.0f, 6.0f, 3.0f);
        Vec3f source(1, 1, 0);
        Vec3f listener(-1, -1, 0);

        RayTracer rt;
        auto contributions = rt.trace(source, listener, walls, 1000, 0.5f);

        QVERIFY(!contributions.empty());
    }

    void testRoomImpulseResponse() {
        auto walls = createBoxRoom(6.0f, 6.0f, 3.0f);
        Vec3f source(1, 1, 0);
        Vec3f listener(-1, -1, 0);

        ImageSourceMethod ism;
        auto imageSources = ism.compute(source, listener, walls, 2);

        RayTracer rt;
        auto rayContribs = rt.trace(source, listener, walls, 500);

        RoomImpulseResponse rir;
        auto impulse = rir.compute(imageSources, rayContribs, 44100);

        QVERIFY(!impulse.empty());
        QVERIFY(rir.duration() > 0.0f);
    }

    void testConvolution() {
        std::vector<float> signal = {1, 0, 0, 0, 0};
        std::vector<float> impulse = {0.5f, 0.3f, 0.1f};

        auto result = SignalProcessing::convolve(signal, impulse);
        QCOMPARE(result.size(), size_t(7));
        QVERIFY(std::abs(result[0] - 0.5f) < 0.01f);
        QVERIFY(std::abs(result[1] - 0.3f) < 0.01f);
        QVERIFY(std::abs(result[2] - 0.1f) < 0.01f);
    }

    void testFFTConvolution() {
        std::vector<float> signal = {1, 0, 0, 0, 0, 0, 0, 0};
        std::vector<float> impulse = {0.5f, 0.3f, 0.1f};

        auto result = SignalProcessing::fftConvolve(signal, impulse);
        QVERIFY(result.size() >= 3);
        QVERIFY(std::abs(result[0] - 0.5f) < 0.05f);
        QVERIFY(std::abs(result[1] - 0.3f) < 0.05f);
        QVERIFY(std::abs(result[2] - 0.1f) < 0.05f);
    }

    void testNormalize() {
        std::vector<float> signal = {0.5f, -1.0f, 0.3f};
        SignalProcessing::normalize(signal, 0.95f);
        float maxAbs = 0;
        for (float s : signal) maxAbs = std::max(maxAbs, std::abs(s));
        QVERIFY(std::abs(maxAbs - 0.95f) < 0.01f);
    }
};

QTEST_MAIN(TestAcousticSimulation)
#include "test_acoustic_simulation.moc"
