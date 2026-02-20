#include "AcousticSimulator.h"
#include "Wall.h"
#include "ImageSourceMethod.h"
#include "RayTracer.h"
#include "RoomImpulseResponse.h"
#include "audio/AudioFile.h"
#include "audio/SignalProcessing.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>

namespace prs {

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
    float scattering)
{
    Q_UNUSED(roomCenter);

    if (!scene.hasMinimumObjects()) {
        qWarning() << "Need at least 1 source and 1 listener";
        return {};
    }

    qInfo() << "=== ACOUSTIC SIMULATION START ===";
    qInfo() << "Sources:" << scene.soundSourceCount()
            << "Listeners:" << scene.listenerCount();

    // Create output directory
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString outputDir = QDir("sounds/simulations").filePath("simulation_" + timestamp);
    QDir().mkpath(outputDir);

    // Save scene config
    scene.saveToFile(QDir(outputDir).filePath("scene.json"));

    // Build walls from mesh triangles
    std::vector<Wall> walls;
    for (auto& wi : wallsFromRender) {
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
            wall.energyAbsorption = energyAbsorption;
            wall.scattering       = scattering;

            if (wall.area() > 1e-8f)
                walls.push_back(wall);
        }
    }

    qInfo() << "Built" << walls.size() << "wall triangles";

    int successCount = 0;

    for (int si = 0; si < scene.soundSourceCount(); ++si) {
        auto* source = scene.getSoundSource(si);
        if (!source) continue;

        qInfo() << "Processing source" << (si + 1) << "/" << scene.soundSourceCount()
                << QString::fromStdString(source->name);

        // Load audio
        AudioFile audioFile;
        if (!audioFile.load(QString::fromStdString(source->audioFile))) {
            qWarning() << "Failed to load audio:" << QString::fromStdString(source->audioFile);
            continue;
        }
        audioFile.applyVolume(source->volume);
        int fs = audioFile.sampleRate();

        Vec3f sourcePos = source->position;

        // For each listener, compute RIR and convolve
        for (int li = 0; li < scene.listenerCount(); ++li) {
            auto* listener = scene.getListener(li);
            if (!listener) continue;

            Vec3f listenerPos = listener->position;

            // Image Source Method
            ImageSourceMethod ism;
            auto imageSources = ism.compute(sourcePos, listenerPos, walls, maxOrder);

            // Ray Tracing
            RayTracer rt;
            auto rayContributions = rt.trace(sourcePos, listenerPos, walls, nRays);

            // Compute RIR
            RoomImpulseResponse rir;
            auto impulse = rir.compute(imageSources, rayContributions, fs);

            // Convolve source audio with RIR
            auto output = SignalProcessing::fftConvolve(audioFile.samples(), impulse);

            // Normalize to prevent clipping
            SignalProcessing::normalize(output, 0.95f);

            // Save output
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

    qInfo() << "=== SIMULATION COMPLETE ===" << successCount << "source(s)";
    lastSimDir_ = outputDir;
    return outputDir;
}

} // namespace prs
