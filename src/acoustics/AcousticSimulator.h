#pragma once

#include "core/Types.h"
#include "scene/SceneManager.h"
#include "rendering/Viewport3D.h"

#include <QString>
#include <vector>
#include <optional>

namespace prs {

class AcousticSimulator {
public:
    AcousticSimulator(int sampleRate = DEFAULT_SAMPLE_RATE);

    QString simulateScene(
        const SceneManager& scene,
        const std::vector<Viewport3D::WallInfo>& wallsFromRender,
        const Vec3f& roomCenter,
        const std::vector<Vec3f>& modelVertices,
        int maxOrder = DEFAULT_MAX_ORDER,
        int nRays = DEFAULT_N_RAYS,
        float energyAbsorption = DEFAULT_ENERGY_ABSORPTION,
        float scattering = DEFAULT_SCATTERING,
        bool airAbsorption = true);

    QString lastSimulationDir() const { return lastSimDir_; }

private:
    int sampleRate_;
    QString lastSimDir_;
};

} // namespace prs
