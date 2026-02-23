#include "SimulationWorker.h"
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

SimulationWorker::SimulationWorker(const Params& params, QObject* parent)
    : QObject(parent), params_(params) {}

void SimulationWorker::cancel() { cancelled_.store(true); }
bool SimulationWorker::isCancelled() const { return cancelled_.load(); }

void SimulationWorker::process() {
    auto& scene = params_.scene;
    if (!scene.hasMinimumObjects()) {
        emit error("Need at least 1 source and 1 listener");
        return;
    }

    emit progressChanged(0, "Building room geometry...");

    std::vector<Wall> walls;
    for (auto& wi : params_.walls) {
        for (int triIdx : wi.triangleIndices) {
            int baseVert = triIdx * 3;
            if (baseVert + 2 >= static_cast<int>(params_.modelVertices.size())) continue;

            Wall wall;
            wall.triangle.v0 = params_.modelVertices[baseVert];
            wall.triangle.v1 = params_.modelVertices[baseVert + 1];
            wall.triangle.v2 = params_.modelVertices[baseVert + 2];
            Vec3f e1 = wall.triangle.v1 - wall.triangle.v0;
            Vec3f e2 = wall.triangle.v2 - wall.triangle.v0;
            wall.triangle.normal = e1.cross(e2).normalized();
            wall.energyAbsorption = wi.energyAbsorption;
            wall.scattering = wi.scattering;

            if (wall.area() > 1e-8f)
                walls.push_back(wall);
        }
    }

    if (isCancelled()) { emit error("Cancelled"); return; }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString outputDir = QDir("sounds/simulations").filePath("simulation_" + timestamp);
    QDir().mkpath(outputDir);
    scene.saveToFile(QDir(outputDir).filePath("scene.json"));

    int totalPairs = scene.soundSourceCount() * scene.listenerCount();
    int pairsDone = 0;

    for (int si = 0; si < scene.soundSourceCount(); ++si) {
        auto* source = scene.getSoundSource(si);
        if (!source) continue;

        if (isCancelled()) { emit error("Cancelled"); return; }

        emit progressChanged(pairsDone * 100 / std::max(totalPairs, 1),
            QString("Loading audio: %1").arg(QString::fromStdString(source->name)));

        AudioFile audioFile;
        if (!audioFile.load(QString::fromStdString(source->audioFile))) {
            qWarning() << "Failed to load:" << QString::fromStdString(source->audioFile);
            pairsDone += scene.listenerCount();
            continue;
        }
        audioFile.applyVolume(source->volume);
        int fs = audioFile.sampleRate();
        Vec3f sourcePos = source->position;

        for (int li = 0; li < scene.listenerCount(); ++li) {
            if (isCancelled()) { emit error("Cancelled"); return; }

            auto* listener = scene.getListener(li);
            if (!listener) continue;

            int pct = (pairsDone * 100) / std::max(totalPairs, 1);
            emit progressChanged(pct, QString("Processing %1 -> %2")
                .arg(QString::fromStdString(source->name),
                     QString::fromStdString(listener->name)));

            Vec3f listenerPos = listener->position;

            ImageSourceMethod ism;
            auto imageSources = ism.compute(sourcePos, listenerPos, walls, params_.maxOrder);

            if (isCancelled()) { emit error("Cancelled"); return; }

            RayTracer rt;
            auto rayContribs = rt.trace(sourcePos, listenerPos, walls, params_.nRays);

            if (isCancelled()) { emit error("Cancelled"); return; }

            RoomImpulseResponse rir;
            auto impulse = rir.compute(imageSources, rayContribs, fs);
            auto output = SignalProcessing::fftConvolve(audioFile.samples(), impulse);
            SignalProcessing::normalize(output, 0.95f);

            QString listenerName = QString::fromStdString(listener->name).replace(' ', '_');
            QString sourceName = QString::fromStdString(source->name).replace(' ', '_');
            QString filename = QString("%1_from_%2.wav").arg(listenerName, sourceName);

            AudioFile outFile;
            outFile.samples() = std::move(output);
            outFile.save(QDir(outputDir).filePath(filename), fs);

            ++pairsDone;
        }
    }

    emit progressChanged(100, "Simulation complete!");
    emit finished(outputDir);
}

} // namespace prs
