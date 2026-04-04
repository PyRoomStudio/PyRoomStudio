#pragma once

#include "core/Types.h"
#include "scene/SceneManager.h"
#include "rendering/Viewport3D.h"

#include <QObject>
#include <QString>
#include <atomic>
#include <vector>

namespace prs {

enum class SimMethod { RayTracing, DG_2D, DG_3D };

class SimulationWorker : public QObject {
    Q_OBJECT

public:
    struct Params {
        SceneManager scene;
        std::vector<Viewport3D::WallInfo> walls;
        Vec3f roomCenter;
        std::vector<Vec3f> modelVertices;
        int sampleRate = DEFAULT_SAMPLE_RATE;
        int maxOrder = DEFAULT_MAX_ORDER;
        int nRays = DEFAULT_N_RAYS;
        float scattering = DEFAULT_SCATTERING;
        bool airAbsorption = true;
        std::vector<int> selectedListenerIndices;
        QString outputDir; // if empty, a timestamped subdirectory is created automatically

        SimMethod method = SimMethod::RayTracing;
        int dgPolyOrder = 3;
        float dgMaxFrequency = 1000.0f;
    };

    explicit SimulationWorker(const Params& params, QObject* parent = nullptr);

    void cancel();
    bool isCancelled() const;

public slots:
    void process();

signals:
    void progressChanged(int percent, const QString& message);
    void finished(const QString& outputDir);
    void error(const QString& message);

private:
    Params params_;
    std::atomic<bool> cancelled_{false};
};

} // namespace prs
