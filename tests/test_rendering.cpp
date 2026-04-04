#include "rendering/Camera.h"
#include "rendering/RayPicking.h"

#include <QtTest/QtTest>

using namespace prs;

class TestRendering : public QObject {
    Q_OBJECT
  private slots:
    void testCameraDefaults() {
        Camera cam;
        QVERIFY(cam.distance() >= 0);
    }

    void testCameraOrbit() {
        Camera cam;
        cam.setDistance(5.0f);
        float initDist = cam.distance();
        cam.orbit(10, 20);
        QCOMPARE(cam.distance(), initDist);
    }

    void testCameraZoom() {
        Camera cam;
        cam.setDistance(5.0f);
        cam.setDistanceLimits(1.0f, 100.0f);
        float before = cam.distance();
        cam.zoom(1.0f, 10.0f);
        QVERIFY(cam.distance() != before);
    }

    void testCameraReset() {
        Camera cam;
        cam.setDistance(5.0f);
        cam.orbit(50, 50);
        cam.reset(10.0f);
        QVERIFY(cam.distance() > 0);
    }

    void testRayTriangleHit() {
        Triangle tri;
        tri.v0 = Vec3f(0, 0, 0);
        tri.v1 = Vec3f(1, 0, 0);
        tri.v2 = Vec3f(0, 1, 0);
        tri.normal = Vec3f(0, 0, 1);

        Vec3f origin(0.2f, 0.2f, 1.0f);
        Vec3f dir(0, 0, -1);
        auto t = RayPicking::rayTriangleIntersect(origin, dir, tri);
        QVERIFY(t.has_value());
        QVERIFY(std::abs(*t - 1.0f) < 1e-4f);
    }

    void testRayTriangleMiss() {
        Triangle tri;
        tri.v0 = Vec3f(0, 0, 0);
        tri.v1 = Vec3f(1, 0, 0);
        tri.v2 = Vec3f(0, 1, 0);
        tri.normal = Vec3f(0, 0, 1);

        Vec3f origin(5, 5, 1);
        Vec3f dir(0, 0, -1);
        auto t = RayPicking::rayTriangleIntersect(origin, dir, tri);
        QVERIFY(!t.has_value());
    }

    void testRaySphereHit() {
        Vec3f origin(0, 0, 5);
        Vec3f dir(0, 0, -1);
        auto t = RayPicking::raySphereIntersect(origin, dir, Vec3f(0, 0, 0), 1.0f);
        QVERIFY(t.has_value());
        QVERIFY(*t > 3.5f && *t < 4.5f);
    }

    void testRaySphereMiss() {
        Vec3f origin(0, 0, 5);
        Vec3f dir(0, 0, -1);
        auto t = RayPicking::raySphereIntersect(origin, dir, Vec3f(10, 10, 0), 1.0f);
        QVERIFY(!t.has_value());
    }
};

QTEST_MAIN(TestRendering)
#include "test_rendering.moc"
