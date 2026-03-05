#include "AcousticSimulator.h"
#include "Wall.h"
#include "Bvh.h"
#include "ImageSourceMethod.h"
#include "RayTracer.h"
#include "RoomImpulseResponse.h"
#include "rendering/MeshSimplifier.h"
#include "audio/AudioFile.h"
#include "audio/SignalProcessing.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>

namespace prs {

namespace {

constexpr int   SIMPLIFICATION_THRESHOLD = 500;
constexpr float SIMPLIFICATION_TARGET_RATIO = 0.3f;

std::vector<AcousticSurface> buildAcousticSurfaces(
    const std::vector<Viewport3D::WallInfo>& wallInfos,
    const std::vector<Vec3f>& modelVertices)
{
    std::vector<AcousticSurface> surfaces;
    for (const auto& wi : wallInfos) {
        Vec3f normalSum = Vec3f::Zero();
        Vec3f centroidSum = Vec3f::Zero();
        float totalArea = 0.0f;
        int validCount = 0;
        Vec3f firstPoint = Vec3f::Zero();

        for (int triIdx : wi.triangleIndices) {
            int base = triIdx * 3;
            if (base + 2 >= static_cast<int>(modelVertices.size())) continue;

            Vec3f v0 = modelVertices[base];
            Vec3f v1 = modelVertices[base + 1];
            Vec3f v2 = modelVertices[base + 2];
            Vec3f e1 = v1 - v0;
            Vec3f e2 = v2 - v0;
            Vec3f n = e1.cross(e2);
            float area = 0.5f * n.norm();
            if (area < 1e-8f) continue;

            normalSum += n.normalized() * area;
            centroidSum += (v0 + v1 + v2) / 3.0f * area;
            totalArea += area;
            if (validCount == 0) firstPoint = v0;
            validCount++;
        }

        if (totalArea < 1e-8f) continue;

        AcousticSurface s;
        s.normal = normalSum.normalized();
        s.centroid = centroidSum / totalArea;
        s.planePoint = firstPoint;
        s.area = totalArea;
        s.energyAbsorption = wi.energyAbsorption;
        s.scattering = wi.scattering;
        surfaces.push_back(s);
    }
    return surfaces;
}

} // namespace

AcousticSimulator::AcousticSimulator(int sampleRate)
    : sampleRate_(sampleRate) {}

QString AcousticSimulator::simulateScene(
    const SceneManager& scene,
    const std::vector<Viewport3D::WallInfo>& wallsFromRender,
    const Vec3f& roomCenter,
    const std::vector<Vec3f>& modelVertices,
    int maxOrder,
    int nRays,
    float energyAbsorption,
    float scattering,
    bool airAbsorption)
{
    Q_UNUSED(roomCenter);

    if (!scene.hasMinimumObjects()) {
        qWarning() << "Need at least 1 source and 1 listener";
        return {};
    }

    QElapsedTimer totalTimer;
    totalTimer.start();

    qInfo() << "=== ACOUSTIC SIMULATION START ===";
    qInfo() << "Sources:" << scene.soundSourceCount()
            << "Listeners:" << scene.listenerCount();

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString outputDir = QDir("sounds/simulations").filePath("simulation_" + timestamp);
    QDir().mkpath(outputDir);
    scene.saveToFile(QDir(outputDir).filePath("scene.json"));

    // Build walls with surface IDs
    QElapsedTimer phaseTimer;
    phaseTimer.start();

    std::vector<Wall> walls;
    std::vector<int> wallSurfaceIds;
    int surfaceIdx = 0;
    for (auto& wi : wallsFromRender) {
        float wallAbsorption = wi.energyAbsorption;
        float wallScattering = wi.scattering;

        for (int triIdx : wi.triangleIndices) {
            int baseVert = triIdx * 3;
            if (baseVert + 2 >= static_cast<int>(modelVertices.size())) continue;

            Wall wall;
            wall.triangle.v0     = modelVertices[baseVert];
            wall.triangle.v1     = modelVertices[baseVert + 1];
            wall.triangle.v2     = modelVertices[baseVert + 2];
            Vec3f e1 = wall.triangle.v1 - wall.triangle.v0;
            Vec3f e2 = wall.triangle.v2 - wall.triangle.v0;
            wall.triangle.normal = e1.cross(e2).normalized();
            wall.energyAbsorption = wallAbsorption;
            wall.scattering       = wallScattering;

            if (wall.area() > 1e-8f) {
                walls.push_back(wall);
                wallSurfaceIds.push_back(surfaceIdx);
            }
        }
        surfaceIdx++;
    }

    qInfo() << "Built" << walls.size() << "wall triangles in" << phaseTimer.elapsed() << "ms";

    // Mesh simplification for dense geometry
    if (static_cast<int>(walls.size()) > SIMPLIFICATION_THRESHOLD) {
        phaseTimer.restart();
        int target = static_cast<int>(walls.size() * SIMPLIFICATION_TARGET_RATIO);
        target = std::max(target, SIMPLIFICATION_THRESHOLD);
        walls = MeshSimplifier::simplify(walls, wallSurfaceIds, target);
        qInfo() << "Simplified to" << walls.size() << "triangles in" << phaseTimer.elapsed() << "ms";
    }

    // Build BVH
    phaseTimer.restart();
    Bvh bvh;
    bvh.build(walls);
    qInfo() << "BVH built in" << phaseTimer.elapsed() << "ms";

    // Build acoustic surfaces for ISM
    phaseTimer.restart();
    auto acousticSurfaces = buildAcousticSurfaces(wallsFromRender, modelVertices);
    qInfo() << "Built" << acousticSurfaces.size() << "acoustic surfaces for ISM in" << phaseTimer.elapsed() << "ms";

    int successCount = 0;

    for (int si = 0; si < scene.soundSourceCount(); ++si) {
        auto* source = scene.getSoundSource(si);
        if (!source) continue;

        qInfo() << "Processing source" << (si + 1) << "/" << scene.soundSourceCount()
                << QString::fromStdString(source->name);

        AudioFile audioFile;
        if (!audioFile.load(QString::fromStdString(source->audioFile))) {
            qWarning() << "Failed to load audio:" << QString::fromStdString(source->audioFile);
            continue;
        }
        audioFile.applyVolume(source->volume);
        int fs = audioFile.sampleRate();

        Vec3f sourcePos = source->position;

        for (int li = 0; li < scene.listenerCount(); ++li) {
            auto* listener = scene.getListener(li);
            if (!listener) continue;

            Vec3f listenerPos = listener->position;

            // ISM with surface-level walls + BVH visibility
            phaseTimer.restart();
            ImageSourceMethod ism;
            auto imageSources = ism.compute(sourcePos, listenerPos, acousticSurfaces, bvh, maxOrder);
            qInfo() << "  ISM:" << phaseTimer.elapsed() << "ms (" << imageSources.size() << "sources)";

            // Ray tracing with BVH
            phaseTimer.restart();
            RayTracer rt;
            auto rayContributions = rt.trace(sourcePos, listenerPos, walls, bvh, nRays, 0.5f, 100, 1e-6f, nullptr, 0.0f, airAbsorption);
            qInfo() << "  Ray tracing:" << phaseTimer.elapsed() << "ms (" << rayContributions.size() << "contributions)";

            // Compute RIR
            phaseTimer.restart();
            RoomImpulseResponse rir;
            auto impulse = rir.compute(imageSources, rayContributions, fs);
            auto output = SignalProcessing::fftConvolve(audioFile.samples(), impulse);
            SignalProcessing::normalize(output, 0.95f);
            qInfo() << "  RIR + convolution:" << phaseTimer.elapsed() << "ms";

            QString listenerName = QString::fromStdString(listener->name).replace(' ', '_');
            QString sourceName   = QString::fromStdString(source->name).replace(' ', '_');
            QString filename     = QString("%1_from_%2.wav").arg(listenerName, sourceName);
            QString outputPath   = QDir(outputDir).filePath(filename);

            AudioFile outFile;
            outFile.samples() = std::move(output);
            if (outFile.save(outputPath, fs)) {
                qInfo() << "  Saved:" << filename;
            } else {
                qWarning() << "  Failed to save:" << filename;
            }
        }

        ++successCount;
    }

    if (successCount == 0) {
        qWarning() << "SIMULATION FAILED - no sources processed";
        return {};
    }

    qInfo() << "=== SIMULATION COMPLETE in" << totalTimer.elapsed() << "ms ===" << successCount << "source(s)";
    lastSimDir_ = outputDir;
    return outputDir;
}

} // namespace prs
