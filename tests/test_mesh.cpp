#include <QtTest/QtTest>
#include <QTemporaryFile>
#include "rendering/MeshData.h"
#include "rendering/SurfaceGrouper.h"

using namespace prs;

static QByteArray createMinimalSTL() {
    // Single triangle STL: 80 header + 4 count + 50 per triangle
    QByteArray data(80, '\0');  // header
    uint32_t count = 1;
    data.append(reinterpret_cast<const char*>(&count), 4);

    float tri[12] = {
        0, 0, 1,    // normal
        0, 0, 0,    // v0
        1, 0, 0,    // v1
        0, 1, 0,    // v2
    };
    data.append(reinterpret_cast<const char*>(tri), 48);
    uint16_t attr = 0;
    data.append(reinterpret_cast<const char*>(&attr), 2);

    return data;
}

static QByteArray createBoxSTL() {
    // 12 triangles forming a unit cube
    QByteArray data(80, '\0');
    uint32_t count = 12;
    data.append(reinterpret_cast<const char*>(&count), 4);

    auto addTri = [&](float nx, float ny, float nz,
                       float x0, float y0, float z0,
                       float x1, float y1, float z1,
                       float x2, float y2, float z2) {
        float tri[12] = {nx, ny, nz, x0, y0, z0, x1, y1, z1, x2, y2, z2};
        data.append(reinterpret_cast<const char*>(tri), 48);
        uint16_t attr = 0;
        data.append(reinterpret_cast<const char*>(&attr), 2);
    };

    // Front  (z=1)
    addTri(0,0,1, 0,0,1, 1,0,1, 1,1,1);
    addTri(0,0,1, 0,0,1, 1,1,1, 0,1,1);
    // Back (z=0)
    addTri(0,0,-1, 1,0,0, 0,0,0, 0,1,0);
    addTri(0,0,-1, 1,0,0, 0,1,0, 1,1,0);
    // Right (x=1)
    addTri(1,0,0, 1,0,0, 1,0,1, 1,1,1);
    addTri(1,0,0, 1,0,0, 1,1,1, 1,1,0);
    // Left (x=0)
    addTri(-1,0,0, 0,0,1, 0,0,0, 0,1,0);
    addTri(-1,0,0, 0,0,1, 0,1,0, 0,1,1);
    // Top (y=1)
    addTri(0,1,0, 0,1,0, 1,1,0, 1,1,1);
    addTri(0,1,0, 0,1,0, 1,1,1, 0,1,1);
    // Bottom (y=0)
    addTri(0,-1,0, 0,0,1, 1,0,1, 1,0,0);
    addTri(0,-1,0, 0,0,1, 1,0,0, 0,0,0);

    return data;
}

class TestMesh : public QObject {
    Q_OBJECT
private slots:
    void testLoadSTL() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        tmp.setFileTemplate(QDir::tempPath() + "/testXXXXXX.stl");
        QVERIFY(tmp.open());
        tmp.write(createMinimalSTL());
        tmp.close();

        MeshData mesh;
        QVERIFY(mesh.loadSTL(tmp.fileName()));
        QCOMPARE(mesh.triangleCount(), 1);
    }

    void testBoundsComputation() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        tmp.setFileTemplate(QDir::tempPath() + "/testXXXXXX.stl");
        QVERIFY(tmp.open());
        tmp.write(createBoxSTL());
        tmp.close();

        MeshData mesh;
        QVERIFY(mesh.loadSTL(tmp.fileName()));
        QCOMPARE(mesh.triangleCount(), 12);

        Vec3f mn = mesh.minBound();
        Vec3f mx = mesh.maxBound();
        QVERIFY(mn.x() < 0.001f && mn.y() < 0.001f && mn.z() < 0.001f);
        QVERIFY(mx.x() > 0.999f && mx.y() > 0.999f && mx.z() > 0.999f);

        float diag = mesh.diagonalSize();
        QVERIFY(diag > 1.7f && diag < 1.8f);
    }

    void testSurfaceGrouping() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        tmp.setFileTemplate(QDir::tempPath() + "/testXXXXXX.stl");
        QVERIFY(tmp.open());
        tmp.write(createBoxSTL());
        tmp.close();

        MeshData mesh;
        QVERIFY(mesh.loadSTL(tmp.fileName()));

        auto edges = SurfaceGrouper::computeFeatureEdges(mesh, 10.0f);
        QVERIFY(!edges.empty());

        auto surfaces = SurfaceGrouper::groupTrianglesIntoSurfaces(mesh, edges);
        QCOMPARE(static_cast<int>(surfaces.size()), 6);

        int totalTris = 0;
        for (auto& s : surfaces) totalTris += static_cast<int>(s.size());
        QCOMPARE(totalTris, 12);
    }

    void testIsClosedBox() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        tmp.setFileTemplate(QDir::tempPath() + "/testXXXXXX.stl");
        QVERIFY(tmp.open());
        tmp.write(createBoxSTL());
        tmp.close();

        MeshData mesh;
        QVERIFY(mesh.loadSTL(tmp.fileName()));
        QVERIFY(mesh.isClosed());
        QCOMPARE(mesh.boundaryEdgeCount(), 0);
    }

    void testIsOpenSingleTriangle() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        tmp.setFileTemplate(QDir::tempPath() + "/testXXXXXX.stl");
        QVERIFY(tmp.open());
        tmp.write(createMinimalSTL());
        tmp.close();

        MeshData mesh;
        QVERIFY(mesh.loadSTL(tmp.fileName()));
        QVERIFY(!mesh.isClosed());
        QVERIFY(mesh.boundaryEdgeCount() > 0);
    }

    void testFilePath() {
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        tmp.setFileTemplate(QDir::tempPath() + "/testXXXXXX.stl");
        QVERIFY(tmp.open());
        tmp.write(createMinimalSTL());
        tmp.close();

        MeshData mesh;
        mesh.loadSTL(tmp.fileName());
        QCOMPARE(mesh.filePath(), tmp.fileName());
    }
};

QTEST_MAIN(TestMesh)
#include "test_mesh.moc"
