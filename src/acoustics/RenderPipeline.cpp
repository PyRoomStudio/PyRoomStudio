#include "RenderPipeline.h"

#include "rendering/MeshData.h"
#include "rendering/SurfaceGrouper.h"

#include <QDir>
#include <QFileInfo>

namespace prs::RenderPipeline {

namespace {

QString resolveRelativePath(const QString& basePath, const QString& candidate) {
    if (candidate.isEmpty())
        return {};
    QFileInfo info(candidate);
    if (info.isAbsolute())
        return info.filePath();
    return QFileInfo(basePath).dir().filePath(candidate);
}

SimMethod toSimMethod(RenderMethod method) {
    switch (method) {
        case RenderMethod::DG_2D:
            return SimMethod::DG_2D;
        case RenderMethod::DG_3D:
            return SimMethod::DG_3D;
        case RenderMethod::RayTracing:
        default:
            return SimMethod::RayTracing;
    }
}

std::vector<Viewport3D::WallInfo> buildWalls(const MeshData& mesh, const ProjectData& project) {
    auto featureEdges = SurfaceGrouper::computeFeatureEdges(mesh, 10.0f);
    auto surfaces = SurfaceGrouper::groupTrianglesIntoSurfaces(mesh, featureEdges);
    std::vector<Viewport3D::WallInfo> walls;
    walls.reserve(surfaces.size());

    for (int si = 0; si < static_cast<int>(surfaces.size()); ++si) {
        Viewport3D::WallInfo wi;
        wi.triangleIndices.assign(surfaces[si].begin(), surfaces[si].end());
        if (si < static_cast<int>(project.surfaceMaterials.size()) && project.surfaceMaterials[si].has_value()) {
            wi.absorption = project.surfaceMaterials[si]->absorption;
            wi.scattering = project.surfaceMaterials[si]->scattering;
        }
        walls.push_back(std::move(wi));
    }

    return walls;
}

SceneManager buildScene(const ProjectData& project, const QString& projectPath, const QString& defaultAudioFile) {
    SceneManager scene;
    const float scale = project.scaleFactor;
    const QString fallbackAudio = resolveRelativePath(projectPath, defaultAudioFile);

    int sourceIndex = 0;
    int listenerIndex = 0;
    for (const auto& pt : project.placedPoints) {
        const Vec3f pos = pt.getPosition() * scale;
        if (pt.pointType == POINT_TYPE_SOURCE) {
            const QString audio = resolveRelativePath(projectPath, QString::fromStdString(pt.audioFile));
            const std::string file = audio.isEmpty() ? fallbackAudio.toStdString() : audio.toStdString();
            const std::string name = pt.name.empty() ? "Source " + std::to_string(++sourceIndex) : pt.name;
            scene.addSoundSource(pos, file, pt.volume, name);
        } else if (pt.pointType == POINT_TYPE_LISTENER) {
            const std::string name = pt.name.empty() ? "Listener " + std::to_string(++listenerIndex) : pt.name;
            scene.addListener(pos, name, pt.getForwardDirection());
        }
    }

    return scene;
}

} // namespace

bool buildSimulationParams(const QString& projectPath, const ProjectData& project, const RenderOptions& options,
                           const std::vector<int>& selectedListenerIndices, const QString& outputDir,
                           SimulationWorker::Params* params, QString* error) {
    if (!params) {
        if (error)
            *error = "internal error: params output is null";
        return false;
    }

    const QString meshPath = resolveRelativePath(projectPath, project.stlFilePath);
    if (meshPath.isEmpty()) {
        if (error)
            *error = "project does not specify a room mesh";
        return false;
    }

    MeshData mesh;
    if (!mesh.load(meshPath)) {
        if (error)
            *error = QString("failed to load room mesh: %1").arg(meshPath);
        return false;
    }

    params->scene = buildScene(project, projectPath, project.soundSourceFile);
    params->walls = buildWalls(mesh, project);
    params->roomCenter = mesh.center() * project.scaleFactor;
    params->modelVertices = mesh.scaledFlatVertices(project.scaleFactor);
    params->sampleRate = options.sampleRate;
    params->maxOrder = options.maxOrder;
    params->nRays = options.nRays;
    params->scattering = options.scattering;
    params->airAbsorption = options.airAbsorption;
    params->selectedListenerIndices = selectedListenerIndices;
    params->method = toSimMethod(options.method);
    params->dgPolyOrder = options.dgPolyOrder;
    params->dgMaxFrequency = options.dgMaxFrequency;
    params->outputDir = outputDir;

    return true;
}

} // namespace prs::RenderPipeline
