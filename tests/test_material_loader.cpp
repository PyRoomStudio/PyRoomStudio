#include "core/MaterialLoader.h"
#include "utils/ResourcePath.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest/QtTest>

using namespace prs;

class TestMaterialLoader : public QObject {
    Q_OBJECT
  private slots:
    void testLoadFromFileReturnsMaterial();
    void testLoadFromFileRejectsMalformedJson();
    void testLoadFromDirectoryBuildsCategories();
    void testResourcePathPrefersExistingAppDir();
    void testResolveMaterialTexturePathFallbacks();
};

static Material makeSampleMaterial() {
    Material mat;
    mat.name = "Sample";
    mat.category = "Concrete";
    mat.thickness = "10mm";
    mat.scattering = 0.2f;
    mat.absorption = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
    mat.color = {128, 128, 128};
    mat.texturePath = "brick.png";
    return mat;
}

void TestMaterialLoader::testLoadFromFileReturnsMaterial() {
    Material mat = makeSampleMaterial();

    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    tmp.close();

    QVERIFY(MaterialLoader::saveToFile(tmp.fileName(), mat));
    auto result = MaterialLoader::loadFromFile(tmp.fileName());
    QVERIFY(result.has_value());
    QCOMPARE(result->name, mat.name);
    QCOMPARE(result->category, mat.category);
}

void TestMaterialLoader::testLoadFromFileRejectsMalformedJson() {
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    tmp.write("not a json");
    tmp.close();

    auto result = MaterialLoader::loadFromFile(tmp.fileName());
    QVERIFY(!result.has_value());
}

void TestMaterialLoader::testLoadFromDirectoryBuildsCategories() {
    QTemporaryDir dir;
    QDir root(dir.path());
    QVERIFY(root.exists());

    QVERIFY(root.mkpath("brick"));
    QDir brickDir(root.filePath("brick"));
    Material mat = makeSampleMaterial();
    mat.name = "Brick";
    mat.category = "Brick";
    QVERIFY(MaterialLoader::saveToFile(brickDir.filePath("brick.mat"), mat));

    Material other = makeSampleMaterial();
    other.name = "Default";
    QVERIFY(MaterialLoader::saveToFile(root.filePath("uncat.mat"), other));

    auto categories = MaterialLoader::loadFromDirectory(dir.path());
    QCOMPARE(categories.size(), 2);
    bool sawBrick = false;
    bool sawOther = false;
    for (const auto& category : categories) {
        if (category.name == "Brick")
            sawBrick = true;
        if (category.name == "Other")
            sawOther = true;
    }
    QVERIFY(sawBrick);
    QVERIFY(sawOther);
}

void TestMaterialLoader::testResourcePathPrefersExistingAppDir() {
    QString relative = "temp_resource_path_test.bin";
    QDir appDir(QCoreApplication::applicationDirPath());
    QString appFile = appDir.filePath(relative);

    QFile file(appFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("x");
    file.close();

    QString resolved = resourcePath(relative);
    QCOMPARE(resolved, appFile);
    QVERIFY(QFile::remove(appFile));

    QString missing = "no_such_file.bin";
    QCOMPARE(resourcePath(missing), missing);
}

void TestMaterialLoader::testResolveMaterialTexturePathFallbacks() {
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    QString stored = tmp.fileName();
    tmp.close();

    // When the stored path exists, it should be returned unchanged
    QVERIFY(resolveMaterialTexturePath(stored) == stored);

    // When the stored path is empty, return empty string
    QCOMPARE(resolveMaterialTexturePath(QString()), QString());

    // Create a file relative to the application directory and use a relative stored path
    QString fallbackRelative = "fallbacks/temp_texture_fallback.bin";
    QDir appDir(QCoreApplication::applicationDirPath());
    QString fallbackAppFile = appDir.filePath(fallbackRelative);
    QFileInfo fallbackInfo(fallbackAppFile);
    QVERIFY(QDir(fallbackInfo.absolutePath()).mkpath("."));

    QFile fallbackHandle(fallbackAppFile);
    QVERIFY(fallbackHandle.open(QIODevice::WriteOnly));
    fallbackHandle.write("y");
    fallbackHandle.close();

    QVERIFY(QFile::exists(fallbackAppFile));

    QString fallback = resolveMaterialTexturePath(fallbackRelative);
    QVERIFY(fallback == fallbackAppFile || fallback == fallbackRelative);
    QVERIFY(QFile::remove(fallbackAppFile));
}

QTEST_MAIN(TestMaterialLoader)
#include "test_material_loader.moc"
