#pragma once

#include "core/Types.h"
#include "scene/SceneManager.h"
#include "rendering/Viewport3D.h"

#include <QObject>
#include <QString>
#include <atomic>
#include <vector>

namespace prs {

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
        float energyAbsorption = DEFAULT_ENERGY_ABSORPTION;
        float scattering = DEFAULT_SCATTERING;
        bool airAbsorption = true;
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
