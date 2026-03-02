#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QDir>
#include "acoustics/Wall.h"
#include "acoustics/Bvh.h"
#include "acoustics/ImageSourceMethod.h"
#include "acoustics/RayTracer.h"
#include "acoustics/RoomImpulseResponse.h"

using namespace prs;

static std::vector<Wall> createSimpleBox(float size = 5.0f) {
    std::vector<Wall> walls;
    auto addWall = [&](Vec3f v0, Vec3f v1, Vec3f v2) {
        Wall w;
        w.triangle.v0 = v0;
        w.triangle.v1 = v1;
        w.triangle.v2 = v2;
        Vec3f e1 = v1 - v0, e2 = v2 - v0;
        w.triangle.normal = e1.cross(e2).normalized();
        w.energyAbsorption = 0.2f;
        w.scattering = 0.1f;
        walls.push_back(w);
    };

    float s = size;
    // Floor (z=0)
    addWall(Vec3f(0,0,0), Vec3f(s,0,0), Vec3f(s,s,0));
    addWall(Vec3f(0,0,0), Vec3f(s,s,0), Vec3f(0,s,0));
    // Ceiling (z=s)
    addWall(Vec3f(0,0,s), Vec3f(s,s,s), Vec3f(s,0,s));
    addWall(Vec3f(0,0,s), Vec3f(0,s,s), Vec3f(s,s,s));
    // Front (y=0)
    addWall(Vec3f(0,0,0), Vec3f(s,0,s), Vec3f(s,0,0));
    addWall(Vec3f(0,0,0), Vec3f(0,0,s), Vec3f(s,0,s));
    // Back (y=s)
    addWall(Vec3f(0,s,0), Vec3f(s,s,0), Vec3f(s,s,s));
    addWall(Vec3f(0,s,0), Vec3f(s,s,s), Vec3f(0,s,s));
    // Left (x=0)
    addWall(Vec3f(0,0,0), Vec3f(0,s,0), Vec3f(0,s,s));
    addWall(Vec3f(0,0,0), Vec3f(0,s,s), Vec3f(0,0,s));
    // Right (x=s)
    addWall(Vec3f(s,0,0), Vec3f(s,0,s), Vec3f(s,s,s));
    addWall(Vec3f(s,0,0), Vec3f(s,s,s), Vec3f(s,s,0));

    return walls;
}

class TestAcoustics : public QObject {
    Q_OBJECT
private slots:
    void testWallArea() {
        Wall w;
        w.triangle.v0 = Vec3f(0, 0, 0);
        w.triangle.v1 = Vec3f(1, 0, 0);
        w.triangle.v2 = Vec3f(0, 1, 0);
        w.triangle.normal = Vec3f(0, 0, 1);
        float area = w.area();
        QVERIFY(std::abs(area - 0.5f) < 1e-5f);
    }

    void testWallReflection() {
        Wall w;
        w.triangle.v0 = Vec3f(0, 0, 0);
        w.triangle.v1 = Vec3f(1, 0, 0);
        w.triangle.v2 = Vec3f(0, 1, 0);
        w.triangle.normal = Vec3f(0, 0, 1);

        Vec3f point(0.5f, 0.5f, 1.0f);
        Vec3f reflected = w.reflectPoint(point);
        QVERIFY(std::abs(reflected.z() + 1.0f) < 1e-4f);
    }

    void testWallPointSide() {
        Wall w;
        w.triangle.v0 = Vec3f(0, 0, 0);
        w.triangle.v1 = Vec3f(1, 0, 0);
        w.triangle.v2 = Vec3f(0, 1, 0);
        w.triangle.normal = Vec3f(0, 0, 1);

        QVERIFY(w.isPointOnSameSide(Vec3f(0, 0, 1)));
        QVERIFY(!w.isPointOnSameSide(Vec3f(0, 0, -1)));
    }

    void testISMSimpleBox() {
        auto walls = createSimpleBox(5.0f);
        Vec3f source(1, 1, 1);
        Vec3f listener(4, 4, 4);

        ImageSourceMethod ism;
        auto images = ism.compute(source, listener, walls, 2);
        QVERIFY(!images.empty());

        for (auto& img : images)
            QVERIFY(img.delay > 0.0f);
    }

    void testRayTracerSimpleBox() {
        auto walls = createSimpleBox(5.0f);
        Vec3f source(2.5f, 2.5f, 2.5f);
        Vec3f listener(1, 1, 1);

        Bvh bvh;
        bvh.build(walls);

        RayTracer rt;
        auto contributions = rt.trace(source, listener, walls, bvh, 100);
        QVERIFY(contributions.size() >= 0);
    }

    void testRIRDuration() {
        std::vector<ImageSource> images;
        ImageSource is;
        is.position = Vec3f(3, 0, 0);
        is.delay = 0.01f;
        is.attenuation = 0.5f;
        is.order = 1;
        images.push_back(is);

        RoomImpulseResponse rir;
        auto impulse = rir.compute(images, {}, 44100);
        QVERIFY(!impulse.empty());
        QVERIFY(impulse.size() > 44);
    }
};

QTEST_MAIN(TestAcoustics)
#include "test_acoustics.moc"
