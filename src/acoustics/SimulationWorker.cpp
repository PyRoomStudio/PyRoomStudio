#include "SimulationWorker.h"
#include "Wall.h"
#include "ImageSourceMethod.h"
#include "RayTracer.h"
#include "RoomImpulseResponse.h"
#include "rendering/RayPicking.h"
#include "audio/AudioFile.h"
#include "audio/SignalProcessing.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>

namespace prs {

namespace {

constexpr float HEAD_RADIUS  = 0.0875f;
constexpr float EAR_OFFSET   = 0.075f;

void earPositions(const Listener* listener, Vec3f& headCenter, Vec3f& leftEar, Vec3f& rightEar) {
    if (!listener) return;
    headCenter = listener->position;
    Vec3f forward(0.0f, 0.0f, -1.0f);
    if (listener->orientation.has_value()) {
        const Vec3f& o = listener->orientation.value();
        if (o.squaredNorm() > 1e-12f)
            forward = o.normalized();
    }
    Vec3f up(0.0f, 1.0f, 0.0f);
    Vec3f right = forward.cross(up);
    if (right.squaredNorm() < 1e-12f)
        right = Vec3f(1.0f, 0.0f, 0.0f);
    else
        right.normalize();
    leftEar  = headCenter - EAR_OFFSET * right;
    rightEar = headCenter + EAR_OFFSET * right;
}

std::vector<ImageSource> filterByHeadOcclusion(const std::vector<ImageSource>& sources,
                                                 const Vec3f& earPos, const Vec3f& headCenter) {
    std::vector<ImageSource> out;
    for (const auto& is : sources) {
        if (!RayPicking::segmentPassesThroughSphere(is.position, earPos, headCenter, HEAD_RADIUS))
            out.push_back(is);
    }
    return out;
}

} // namespace

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
    int mixedFs = 44100;

    struct MixedStereo { std::vector<float> left; std::vector<float> right; };
    std::vector<MixedStereo> mixedPerListener(static_cast<size_t>(scene.listenerCount()));

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
        mixedFs = fs;
        Vec3f sourcePos = source->position;
        const std::vector<float>& inputSamples = audioFile.samples();
        if (inputSamples.empty()) {
            qWarning() << "No samples loaded for source, skipping";
            pairsDone += scene.listenerCount();
            continue;
        }

        for (int li = 0; li < scene.listenerCount(); ++li) {
            if (isCancelled()) { emit error("Cancelled"); return; }

            auto* listener = scene.getListener(li);
            if (!listener) continue;

            int pct = (pairsDone * 100) / std::max(totalPairs, 1);
            emit progressChanged(pct, QString("Processing %1 -> %2 (stereo)")
                .arg(QString::fromStdString(source->name),
                     QString::fromStdString(listener->name)));

            Vec3f headCenter, leftEar, rightEar;
            earPositions(listener, headCenter, leftEar, rightEar);

            ImageSourceMethod ism;
            auto isLeft  = ism.compute(sourcePos, leftEar,  walls, params_.maxOrder);
            auto isRight = ism.compute(sourcePos, rightEar, walls, params_.maxOrder);
            isLeft  = filterByHeadOcclusion(isLeft,  leftEar,  headCenter);
            isRight = filterByHeadOcclusion(isRight, rightEar, headCenter);

            if (isCancelled()) { emit error("Cancelled"); return; }

            RayTracer rt;
            auto rayLeft  = rt.trace(sourcePos, leftEar,  walls, params_.nRays, 0.5f, 100, 1e-6f, &headCenter, HEAD_RADIUS);
            auto rayRight = rt.trace(sourcePos, rightEar, walls, params_.nRays, 0.5f, 100, 1e-6f, &headCenter, HEAD_RADIUS);

            if (isCancelled()) { emit error("Cancelled"); return; }

            RoomImpulseResponse rir;
            auto impulseLeft  = rir.compute(isLeft,  rayLeft,  fs);
            auto impulseRight = rir.compute(isRight, rayRight, fs);

            auto outLeft  = SignalProcessing::fftConvolve(inputSamples, impulseLeft);
            auto outRight = SignalProcessing::fftConvolve(inputSamples, impulseRight);
            if (outLeft.empty() && outRight.empty())
                continue;
            if (outLeft.empty()) outLeft.resize(outRight.size(), 0.0f);
            if (outRight.empty()) outRight.resize(outLeft.size(), 0.0f);
            size_t outLen = std::max(outLeft.size(), outRight.size());
            outLeft.resize(outLen, 0.0f);
            outRight.resize(outLen, 0.0f);

            SignalProcessing::normalize(outLeft,  0.95f);
            SignalProcessing::normalize(outRight, 0.95f);

            QString listenerName = QString::fromStdString(listener->name).replace(' ', '_');
            QString sourceName = QString::fromStdString(source->name).replace(' ', '_');
            QString filename = QString("%1_from_%2.wav").arg(listenerName, sourceName);
            if (!AudioFile::saveStereo(QDir(outputDir).filePath(filename), fs, outLeft, outRight))
                qWarning() << "Failed to write" << filename;

            MixedStereo& mix = mixedPerListener[static_cast<size_t>(li)];
            size_t n = std::max(mix.left.size(), outLen);
            mix.left.resize(n, 0.0f);
            mix.right.resize(n, 0.0f);
            for (size_t i = 0; i < outLeft.size(); ++i) mix.left[i] += outLeft[i];
            for (size_t i = 0; i < outRight.size(); ++i) mix.right[i] += outRight[i];

            ++pairsDone;
        }
    }

    if (scene.soundSourceCount() > 1) {
        for (int li = 0; li < scene.listenerCount(); ++li) {
            auto* listener = scene.getListener(li);
            if (!listener || (mixedPerListener[static_cast<size_t>(li)].left.empty())) continue;
            MixedStereo& m = mixedPerListener[static_cast<size_t>(li)];
            float maxVal = 0.0f;
            for (float s : m.left)  maxVal = std::max(maxVal, std::abs(s));
            for (float s : m.right) maxVal = std::max(maxVal, std::abs(s));
            if (maxVal > 1.0f) {
                for (float& s : m.left)  s /= maxVal;
                for (float& s : m.right) s /= maxVal;
            }
            QString listenerName = QString::fromStdString(listener->name).replace(' ', '_');
            AudioFile::saveStereo(QDir(outputDir).filePath(listenerName + "_mixed.wav"), mixedFs, m.left, m.right);
        }
    }

    emit progressChanged(100, "Simulation complete!");
    emit finished(outputDir);
}

} // namespace prs
